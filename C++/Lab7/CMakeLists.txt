cmake_minimum_required(VERSION 3.20)
project(
   labwork7 
   LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

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

FetchContent_MakeAvailable(cpr nlohmann_json)

add_library(app_lib
   src/app_main.cpp
   src/api_work.cpp
   src/cache.cpp
   src/env_config.cpp
   src/main_entry.cpp
   src/print.cpp
)
target_include_directories(app_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(app_lib PUBLIC
   cpr::cpr
   nlohmann_json::nlohmann_json
)

add_executable(routes_app
   bin/main.cpp
)

target_include_directories(routes_app PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(routes_app PRIVATE app_lib)

include(CTest)
if(BUILD_TESTING)
   add_executable(unit_tests
      tests/test_main.cpp
      tests/test_cache.cpp
      tests/test_api.cpp
      tests/test_cli.cpp
      tests/test_print_extra.cpp
      tests/test_app_main.cpp
      tests/test_main_entry.cpp
   )
   target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
   target_link_libraries(unit_tests PRIVATE app_lib)
   if(NOT TARGET GTest::gtest)
      add_library(GTest::gtest INTERFACE IMPORTED)
   endif()
   target_link_libraries(unit_tests PRIVATE GTest::gtest)
   add_test(NAME unit_tests COMMAND unit_tests)
endif()

include(${CMAKE_SOURCE_DIR}/tests/coverage.cmake)

if(TARGET coverage)
   if(TARGET unit_tests)
      add_dependencies(coverage unit_tests)
   endif()
   if(TARGET routes_app)
      add_dependencies(coverage routes_app)
   endif()
endif()
