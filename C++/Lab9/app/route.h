#pragma once

#include "places.h"

#include <string>

namespace route {

constexpr double kMetersInKm = 1000.0;
constexpr double kMinutesInHour = 60.0;

constexpr double kFootSpeedKmh = 4.0;
constexpr double kBikeSpeedKmh = 12.0;
constexpr double kCarSpeedKmh = 35.0;

struct Route {
    double foot_time;
    double bike_time;
    double car_time;
};

double Time(double distance_m, double speed_kmh) {
    return (distance_m / kMetersInKm) / speed_kmh * kMinutesInHour;
}

inline Route GetTime(double distance_m) {
    return Route{Time(distance_m, kFootSpeedKmh), Time(distance_m, kBikeSpeedKmh), Time(distance_m, kCarSpeedKmh),};
}

} //namespace route