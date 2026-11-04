#pragma once

#include "../lib/include/database.h"

#include <string>
#include <vector>

void ReadLines(Database& database);

void ParseLine(Database& database, const std::vector<std::string>& parsed_line);

void ApplyCommandLine(Database& database, int argc, char* argv[]);