#pragma once

#include <string> 
#include <stdexcept>

namespace geo {

struct Point {
    double latitude;
    double longitude;
};

bool ValidPoint(const Point& point);

void Parse(const std::string& coords, Point& point);

Point ApiWorkLocation();

} //namespace geo