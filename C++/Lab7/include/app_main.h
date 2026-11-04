#pragma once

#include "api_work.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

int AppMain(const std::vector<std::string>& args, std::istream& in, std::ostream& out, std::ostream& err,
            std::shared_ptr<Http> http);
