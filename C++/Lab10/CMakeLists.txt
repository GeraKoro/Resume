cmake_minimum_required(VERSION 3.20)
project(labwork10 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)
include(CTest)
enable_testing()

add_library(database
    lib/src/database_general.cpp
    lib/src/database_memory.cpp
    lib/src/database_string.cpp
    lib/src/database_list.cpp
    lib/src/database_set.cpp
    lib/src/database_geo.cpp
)
target_include_directories(database PUBLIC lib/include)

add_executable(valkey bin/main.cpp bin/parser.cpp)
target_link_libraries(valkey PRIVATE database)

add_subdirectory(tests)