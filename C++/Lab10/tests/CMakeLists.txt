find_package(GTest QUIET CONFIG)

if(NOT GTest_FOUND)
    include(FetchContent)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(database_tests test_database.cpp)
target_link_libraries(database_tests PRIVATE database GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(database_tests)