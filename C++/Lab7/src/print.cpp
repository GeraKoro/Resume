#include "print.h"

#include "objects.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <ostream>
#include <string>

namespace {

std::string Trim(std::string s) {
    const auto n0 = s.find_first_not_of(" \t\r\n");
    if (n0 == std::string::npos) {
        return {};
    }
    const auto n1 = s.find_last_not_of(" \t\r\n");
    return s.substr(n0, n1 - n0 + 1);
}

} // namespace

std::string Print::Destination(const std::string& input) {
    std::string s = Trim(input);
    if (s.empty()) {
        return "NONE";
    }
    std::string lower;
    lower.reserve(s.size());
    for (unsigned char c : s) {
        lower += static_cast<char>(std::tolower(c));
    }
    if (lower == "spb" || lower == "питер" || lower == "петербург") {
        return "Санкт-Петербург";
    }
    if (lower == "msk" || lower == "мск") {
        return "Москва";
    }
    if (lower == "moscow" || lower == "москва") {
        return "Москва";
    }
    if (lower == "pyongyang" || lower == "пхеньян") {
        return "Пхеньян";
    }
    if (lower == "mogadishu" || lower == "могадишо" || lower == "somalia" || lower == "сомали") {
        return "Могадишо";
    }
    return s;
}

void Print::FormatTime(std::ostream& out, const std::string& iso) {
    if (iso.empty() || iso == "NONE") {
        out << "—";
        return;
    }
    std::string s = iso;
    if (s.size() >= 19 && s[10] == 'T') {
        out << s.substr(11, 5) << ' ' << s.substr(0, 10);
        return;
    }
    out << s;
}

void Print::FormatDuration(std::ostream& out, std::size_t minutes) {
    if (minutes == 0) {
        out << "0м";
        return;
    }
    const std::size_t h = minutes / 60;
    const std::size_t m = minutes % 60;
    if (h > 0) {
        out << h << "ч";
    }
    if (m > 0 || h == 0) {
        if (h > 0) {
            out << ' ';
        }
        out << m << "м";
    }
}

void Print::PathInfo(std::ostream& out, const Path& path) {
    out << "  Транспорт: " << path.GetTransportType() << " " << path.GetTransportName() << '\n';
    out << "  Отправление: " << path.GetDepartureStationName() << " — ";
    FormatTime(out, path.GetDepartureTime());
    out << "\n  Прибытие: " << path.GetArrivalStationName() << " — ";
    FormatTime(out, path.GetArrivalTime());
    out << "\n  В пути: ";
    FormatDuration(out, path.GetDuration());
    out << '\n';

    const auto& legs = path.GetConnection();
    if (legs.size() < 2) {
        return;
    }
    out << "  Пересадки по маршруту:\n";
    for (std::size_t i = 0; i + 1 < legs.size(); ++i) {
        const Path& leg = legs[i];
        const Path& next = legs[i + 1];
        out << "    • " << leg.GetArrivalStationName();
        out << ": прибытие ";
        FormatTime(out, leg.GetArrivalTime());
        out << " → отправление ";
        FormatTime(out, next.GetDepartureTime());
        out << " (" << next.GetDepartureStationName() << ")\n";
    }
}

void Print::RouteInfo(std::ostream& out, const Route& route, std::size_t index) {
    out << "\n=== Маршрут #" << (index + 1) << " ===\n";
    out << route.GetDepartureFrom() << " → " << route.GetArrivalTo() << '\n';
    const bool inner_transfer = route.GetFirstPath().GetConnection().size() >= 2;
    if (route.HasTransfer()) {
        out << "Маршрут с пересадками (два участка)\n";
        PathInfo(out, route.GetFirstPath());
        out << "  ---\n";
        if (route.GetSecondPath().has_value()) {
            PathInfo(out, *route.GetSecondPath());
        }
    } else if (inner_transfer) {
        out << "Маршрут с пересадками\n";
        PathInfo(out, route.GetFirstPath());
    } else {
        out << "Маршрут без пересадок\n";
        PathInfo(out, route.GetFirstPath());
    }
    out << "Всего в пути: ";
    FormatDuration(out, route.GetTotalDuration());
    out << '\n';
}

void Print::Routes(std::ostream& out, const std::vector<Route>& routes, std::size_t max_show) {
    if (routes.empty()) {
        out << "Маршруты не найдены.\n";
        return;
    }
    const std::size_t n = std::min(max_show, routes.size());
    out << "Маршрутов: " << routes.size() << ", показано: " << n << "\n\n";
    for (std::size_t i = 0; i < n; ++i) {
        RouteInfo(out, routes[i], i);
    }
}
