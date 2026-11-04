#pragma once

#include "location.h"

#include <string>


namespace weather {

struct Warning {
    bool windy = false;
    bool cold = false;
    bool hot = false;
    bool night = false;
    bool drizzling = false;

    std::string message = "";
};

struct CurrentWeather {
    int weather_code;

    double temperature;
    double apparent_temperature;
    
    double precipitation_mm;
    double cloud_cover;
    double wind_speed;
    
    bool is_day;
    bool bad_outside;

    Warning warning;
};

CurrentWeather GetWeather(const geo::Point& point);

} //namespace weather