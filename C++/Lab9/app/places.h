#pragma once

#include "weather.h"
#include "location.h"

#include <vector>
#include <string>

namespace place {

struct Place {
    std::string name;
    double distance_m;
    bool is_outside;
    std::string type;
    double lat;
    double lon;

    bool operator<(const Place& other) const {
        return this->distance_m < other.distance_m;
    }
};

std::vector<Place> GetPlaces(weather::CurrentWeather& weather, const geo::Point& point);


} //namespace place