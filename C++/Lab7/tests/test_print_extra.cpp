#include "test_framework.h"

#include "objects.h"
#include "print.h"

#include <sstream>
#include <string>
#include <vector>

TEST_CASE("print_format_duration_zero") {
    Print print;
    std::ostringstream out;
    print.FormatDuration(out, 0);
    ASSERT_EQ(out.str(), std::string("0м"));
}

TEST_CASE("print_format_duration_hours_minutes") {
    Print print;
    std::ostringstream o1;
    print.FormatDuration(o1, 90);
    ASSERT_EQ(o1.str(), std::string("1ч 30м"));

    std::ostringstream o2;
    print.FormatDuration(o2, 60);
    ASSERT_TRUE(o2.str().find("ч") != std::string::npos);
}

TEST_CASE("print_format_time_plain") {
    Print print;
    std::ostringstream out;
    print.FormatTime(out, "2026-03-30T14:05:00+03:00");
    ASSERT_TRUE(out.str().find("14:05") != std::string::npos);
    ASSERT_TRUE(out.str().find("2026-03-30") != std::string::npos);
}

TEST_CASE("print_route_marks_transfer_when_two_paths") {
    Print print;
    Path a("train", "1", "A", "a", "H", "h", "t1", "t2", 60);
    Path b("train", "2", "H", "h", "B", "b", "t3", "t4", 60);
    Route r(a, b, "d", "X", "Y");
    std::ostringstream out;
    print.RouteInfo(out, r, 0);
    ASSERT_TRUE(out.str().find("пересадк") != std::string::npos);
}

TEST_CASE("print_route_marks_transfer_from_connection_legs") {
    Print print;
    Path leg1("train", "1", "A", "a", "X", "x", "t1", "t2", 30);
    Path leg2("train", "2", "X", "x", "B", "b", "t3", "t4", 40);
    Path p("train", "1 / 2", "A", "a", "B", "b", "t1", "t4", 70);
    p.SetConnection(std::vector<Path>{leg1, leg2});
    Route r(p, "d", "X", "Y");
    std::ostringstream out;
    print.RouteInfo(out, r, 0);
    ASSERT_TRUE(out.str().find("пересадк") != std::string::npos);
}

TEST_CASE("print_path_shows_transfer_points_from_legs") {
    Print print;
    Path leg1("train", "100", "A", "a", "X", "x", "2026-03-30T10:00:00+03:00", "2026-03-30T11:00:00+03:00", 60);
    Path leg2("train", "200", "X", "x", "B", "b", "2026-03-30T11:30:00+03:00", "2026-03-30T13:30:00+03:00", 120);
    Path p("train", "100 / 200", "A", "a", "B", "b", "2026-03-30T10:00:00+03:00", "2026-03-30T13:30:00+03:00", 210);
    p.SetConnection(std::vector<Path>{leg1, leg2});
    std::ostringstream out;
    print.PathInfo(out, p);
    ASSERT_TRUE(out.str().find("Пересадки") != std::string::npos);
    ASSERT_TRUE(out.str().find("X") != std::string::npos);
}

TEST_CASE("print_routes_respects_limit") {
    Print print;
    std::vector<Route> routes;
    for (int i = 0; i < 7; ++i) {
        Path p("train", "n", "A", "a", "B", "b", "t0", "t1", 10);
        routes.emplace_back(p, "d", "X", "Y");
    }
    std::ostringstream out;
    print.Routes(out, routes, 3);
    const std::string s = out.str();
    ASSERT_TRUE(s.find("показано: 3") != std::string::npos);
    ASSERT_TRUE(s.find("Маршрут #3") != std::string::npos);
    ASSERT_TRUE(s.find("Маршрут #4") == std::string::npos);
}
