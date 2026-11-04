#include "test_framework.h"

#include "objects.h"
#include "print.h"

#include <sstream>
#include <string>
#include <vector>

TEST_CASE("cli_destination_aliases") {
    Print print;
    ASSERT_EQ(print.Destination("питер"), std::string("Санкт-Петербург"));
    ASSERT_EQ(print.Destination("мск"), std::string("Москва"));
}

TEST_CASE("cli_printroutes_nonempty_header") {
    Print print;
    Path p("train", "42", "A", "cA", "B", "cB", "2026-03-30T10:00:00+03:00", "2026-03-30T12:00:00+03:00", 120);
    Route r(p, "2026-03-30", "X", "Y");
    std::vector<Route> v{r};

    std::ostringstream out;
    print.Routes(out, v, 1);
    const std::string s = out.str();
    ASSERT_TRUE(s.find("Маршрут #1") != std::string::npos);
    ASSERT_TRUE(s.find("Маршрут без пересадок") != std::string::npos);
}
