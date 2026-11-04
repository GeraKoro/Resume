#pragma once

#include "objects.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

class Print {
public:
    std::string Destination(const std::string& input);
    void FormatTime(std::ostream& out, const std::string& iso);
    void FormatDuration(std::ostream& out, std::size_t minutes);
    void PathInfo(std::ostream& out, const Path& path);
    void RouteInfo(std::ostream& out, const Route& route, std::size_t index);
    void Routes(std::ostream& out, const std::vector<Route>& routes, std::size_t max_show);
};
