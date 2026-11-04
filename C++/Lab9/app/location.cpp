#include "location.h"

#include <iostream>
#include <string>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace geo {


bool ValidPoint(const Point& point) {
    if ((point.latitude < -90) || (point.latitude > 90) || (point.longitude < -180) || (point.longitude > 180)) {
        return false;
    }
    return true;
}

void Parse(const std::string& coords, Point& point) {
    size_t pos = coords.find(' ');
    if (pos == std::string::npos) {
        throw std::out_of_range("Введите координаты через пробел, сначала широту, потом долготу");
    }
    std::string lat = coords.substr(0, pos);
    std::string lon = coords.substr(pos + 1);
    point.latitude = std::stod(lat);
    point.longitude = std::stod(lon);
    if (!ValidPoint(point)) {
        throw std::invalid_argument("Координаты некорректны");
    }
}

Point ApiWorkLocation() {
    auto response = cpr::Get(
        cpr::Url{"https://ipwho.is/"},
        cpr::Timeout{15000}
    );
    if (response.status_code != 200) {
        throw std::runtime_error(
            "Не удалось определить местоположение (HTTP " + std::to_string(response.status_code) + ")");
    }

    json data = json::parse(response.text);
    if (!data.value("success", false)) {
        throw std::runtime_error("Геолокация: " + data.value("message", std::string{"unknown"}));
    }

    double lat = data.at("latitude").get<double>();
    double lon = data.at("longitude").get<double>();

    std::string city = data.value("city", "Не удалось распознать город");
    std::string country = data.value("country", "Не удалось распознать страну");

    std::cout << "Ваше местоположение: " << city << ", " << country << '\n';
    std::cout << "Ваши координаты: " << lat << " " << lon << '\n';
    std::cout << "Верно? (да/нет): ";

    std::string answer;
    std::getline(std::cin, answer);

    if (answer == "нет" || answer == "Нет" || answer == "no" || answer == "n" || answer == "No" || answer == "N") {
        std::cout << "Введите ваши координаты (ширина, долгота): ";
        std::string coords;
        std::getline(std::cin, coords);
        Point point{};
        Parse(coords, point);
        return point;
    }
    return Point{lat, lon};
}

} //namespace geo