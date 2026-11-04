#include "weather.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <string>
#include <stdexcept>

using json = nlohmann::json;

constexpr const char* kBaseUrl = "https://api.open-meteo.com/v1/forecast?";

namespace weather {

CurrentWeather GetWeather(const geo::Point& point) {
    std::ostringstream url;
    url << kBaseUrl 
    << "latitude=" << point.latitude
    << "&longitude=" << point.longitude
    << "&wind_speed_unit=ms" 
    << "&current=temperature_2m,apparent_temperature,cloud_cover,wind_speed_10m"
    << ",precipitation,weather_code,is_day"
    << "&timezone=auto";

    const std::string str_url = url.str();

    auto response = cpr::Get(cpr::Url{str_url});
    if (response.status_code != 200) {
        throw std::runtime_error("Не получилось получить погоду");
    }

    json ans = json::parse(response.text);
    const json& current = ans.at("current");

    CurrentWeather current_weather{};

    current_weather.temperature = current.at("temperature_2m").get<double>();
    current_weather.apparent_temperature = current.at("apparent_temperature").get<double>();
    current_weather.cloud_cover = current.at("cloud_cover").get<double>();
    current_weather.wind_speed = current.at("wind_speed_10m").get<double>();
    current_weather.precipitation_mm = current.at("precipitation").get<double>();
    current_weather.weather_code = current.at("weather_code").get<int>();
    current_weather.is_day = current.at("is_day").get<int>() != 0;

    if ((current_weather.weather_code > 53 || (current_weather.wind_speed > 15) || (current_weather.precipitation_mm > 2))) {
        current_weather.bad_outside = true;
    }
    else {
        current_weather.bad_outside = false;
    }

    if (current_weather.apparent_temperature < 10) {
        current_weather.warning.cold = true;
        current_weather.warning.message += "Может быть холодно на улице, оденьтесь теплее! ";
    }
    if (current_weather.wind_speed > 10) {
        current_weather.warning.windy = true;
        current_weather.warning.message += "Может быть ветренно на улице! ";
    }
    if (current_weather.apparent_temperature > 35) {
        current_weather.warning.hot = true;
        current_weather.warning.message += "Может быть жарко на улице! ";
    }
    if (!current_weather.is_day) {
        current_weather.warning.night = true;
        current_weather.warning.message += "На улице ночь, будьте осторожнее! ";
    }
    if (current_weather.weather_code >= 45) {
        current_weather.warning.drizzling = true;
        current_weather.warning.message += "Может немного моросить, ничего страшного, но возьмите зонтик, на всякий случай! ";
    }

    return current_weather;
}

} //namespace weather