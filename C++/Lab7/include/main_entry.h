#pragma once

#include "api_work.h"

#include <iostream>
#include <memory>
#include <string>

int MainEntry(int argc, char* argv[], std::istream& in, std::ostream& out, std::ostream& err, std::shared_ptr<Http> http);
