cmake_minimum_required(VERSION 3.20)
project(labwork9 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_Declare(
   cpr
   GIT_REPOSITORY https://github.com/libcpr/cpr.git
   GIT_TAG 1.12.0
)

FetchContent_Declare(
   nlohmann_json
   GIT_REPOSITORY https://github.com/nlohmann/json.git
   GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(Catch2 cpr nlohmann_json)

add_library(app_lib
   app/location.cpp
   app/weather.cpp
   app/places.cpp
)
target_include_directories(app_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(app_lib PUBLIC
   cpr::cpr
   nlohmann_json::nlohmann_json
)
target_compile_options(app_lib PRIVATE -Wall -Wextra)

add_executable(run_tests tests/test_ttask.cpp)
target_include_directories(run_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(run_tests PRIVATE -Wall -Wextra)
target_link_libraries(run_tests PRIVATE Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(run_tests)

add_executable(app bin/main.cpp)
target_link_libraries(app PRIVATE app_lib)
