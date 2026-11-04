#include "places.h"
#include "weather.h"
#include "location.h"

#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace place {

namespace {

std::string GetName(const json& tags) {
    if (!tags.contains("name")) {
        return {};
    }
    const json& name = tags["name"];
    if (name.is_string()) {
        return name.get<std::string>();
    }
    if (name.is_object() && !name.empty()) {
        for (auto it = name.begin(); it != name.end(); ++it) {
            if (it.value().is_string()) {
                return it.value().get<std::string>();
            }
        }
    }
    return {};
}

std::string GetType(const json& tags) {
    if (tags.contains("amenity") && tags["amenity"].is_string()) {
        return tags["amenity"].get<std::string>();
    }
    if (tags.contains("leisure") && tags["leisure"].is_string()) {
        return tags["leisure"].get<std::string>();
    }
    if (tags.contains("tourism") && tags["tourism"].is_string()) {
        return tags["tourism"].get<std::string>();
    }
    return {};
}

std::string RuLabel(const std::string& osm_type) {
    if (osm_type == "playground") {
        return "Детская площадка";
    }
    if (osm_type == "park") {
        return "Парк";
    }
    if (osm_type == "garden") {
        return "Сад";
    }
    if (osm_type == "attraction") {
        return "Достопримечательность";
    }
    if (osm_type == "viewpoint") {
        return "Смотровая площадка";
    }
    if (osm_type == "cafe") {
        return "Кафе";
    }
    if (osm_type == "restaurant") {
        return "Ресторан";
    }
    if (osm_type == "cinema") {
        return "Кинотеатр";
    }
    if (osm_type == "theatre" || osm_type == "theater") {
        return "Театр";
    }
    if (osm_type == "library") {
        return "Библиотека";
    }
    if (osm_type == "museum") {
        return "Музей";
    }
    return "Место";
}

std::string DisplayName(const json& tags) {
    const std::string name = GetName(tags);
    if (!name.empty()) {
        return name;
    }
    return RuLabel(GetType(tags));
}

} // namespace

double DistFormula(double lat1, double lon1, double lat2, double lon2) {
    constexpr double kEarthRadius = 6371000.0;
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dLat / 2) * std::sin(dLat / 2)
               + std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0)
               * std::sin(dLon / 2) * std::sin(dLon / 2);
    return kEarthRadius * 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
}

std::vector<Place> ApiWorkPlaces(const geo::Point& point, const std::string& place_types, bool outside, const std::string& place_types_alt = "") {
    constexpr double radius = 2000.0;
    constexpr int kMaxPlaces = 40;

    std::string around = "(around:" + std::to_string(static_cast<int>(radius)) + "," + std::to_string(point.latitude) + "," + std::to_string(point.longitude) + ")";

    std::string query = "[out:json][timeout:25];\n";
    if (place_types_alt.empty()) {
        query += "nwr" + place_types + around + ";\n";
    }
    else {
        query += "(\n  nwr" + place_types + around + ";\n";
        query += "  nwr" + place_types_alt + around + ";\n";
        query += ");\n";
    }
    query += "out center " + std::to_string(kMaxPlaces) + ";\n";

    auto response = cpr::Post(
        cpr::Url{"https://overpass-api.de/api/interpreter"},
        cpr::Payload{{"data", query}},
        cpr::Timeout{120000},
        cpr::Header{{"User-Agent", "labwork9-GeraKoro/1.0 (ITMO lab9)"}}
    );

    if (response.status_code != 200) {
        throw std::runtime_error("Не удалось запросить места рядом (HTTP " + std::to_string(response.status_code) + "): " + response.text);
    }

    json data = json::parse(response.text, nullptr, false);
    if (data.is_discarded()) {
        throw std::runtime_error("Не удалось разобрать ответ Overpass (обрыв соединения или слишком большой ответ)");
    }
    if (!data.contains("elements")) {
        return {};
    }

    std::vector<Place> places;
    places.reserve(data["elements"].size());
    for (const auto& el : data["elements"]) {
        double lat = 0.0;
        double lon = 0.0;
        if (el.contains("lat") && el.contains("lon")) {
            lat = el["lat"].get<double>();
            lon = el["lon"].get<double>();
        }
        else if (el.contains("center")) {
            lat = el["center"]["lat"].get<double>();
            lon = el["center"]["lon"].get<double>();
        }
        else {
            continue;
        }
        double dist = DistFormula(point.latitude, point.longitude, lat, lon);

        std::string type = "Место";
        std::string name = "Место";
        if (el.contains("tags")) {
            const json& tags = el["tags"];
            type = RuLabel(GetType(tags));
            name = DisplayName(tags);
        }

        places.push_back({name, dist, outside, type, lat, lon});
    }

    std::sort(places.begin(), places.end());

    return places;
}

std::vector<Place> GetPlaces(weather::CurrentWeather& weather, const geo::Point& point) {
    const std::string kIndoor = "[\"amenity\"~\"cafe|restaurant|cinema|theatre|library|museum\"]";
    const std::string kOutdoorLeisure = "[\"leisure\"~\"park|garden|playground\"]";
    const std::string kOutdoorTourism = "[\"tourism\"~\"attraction|viewpoint\"]";

    auto PickOutdoor = [&]() {
        return ApiWorkPlaces(point, kOutdoorLeisure, true, kOutdoorTourism);
    };
    auto PickIndoor = [&]() {
        return ApiWorkPlaces(point, kIndoor, false);
    };

    std::vector<Place> places;
    if (weather.bad_outside) {
        places = PickIndoor();
    }
    else if (weather.warning.message != "") {
        std::cout << "Погода может быть неприятной: " << weather.warning.message << "\nЖелаете ли вы провести время на улице";
        std::cout << " или внутри? (in/out): ";

        std::string answer;
        std::getline(std::cin, answer);
        if (answer == "in" || answer == "In" || answer == "Внутри" || answer == "внутри") {
            places = PickIndoor();
        }
        else {
            places = PickOutdoor();
        }
    }
    else {
        places = PickOutdoor();
    }

    if (places.empty() && !weather.bad_outside) {
        places = PickIndoor();
    }

    return places;
}

} //namespace place