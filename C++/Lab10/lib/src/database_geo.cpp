#include "../include/database.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <numbers>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace {

constexpr double kEarthRadius = 6372.8;

double DistFormula(double lat1, double lon1, double lat2, double lon2) {
    constexpr double kRadians = std::numbers::pi / 180.0;
    double d_lat = (lat2 - lat1) * kRadians;
    double d_lon = (lon2 - lon1) * kRadians;
    double a = std::sin(d_lat / 2) * std::sin(d_lat / 2) + std::cos(lat1 * kRadians) * std::cos(lat2 * kRadians) * std::sin(d_lon / 2) * std::sin(d_lon / 2);
    return kEarthRadius * 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
}

double ConvertFromKm(double km, GeoUnit unit) {
    switch (unit) {
        case GeoUnit::kM:
            return km * 1000.0;
        case GeoUnit::kKm:
            return km;
        case GeoUnit::kMi:
            return km / 1.61;
        case GeoUnit::kFt:
            return km * 1000.0 / 0.304;
    }
    return km;
}

double DistFormulaUnit(double lat1, double lon1, double lat2, double lon2, GeoUnit unit) {
    return ConvertFromKm(DistFormula(lat1, lon1, lat2, lon2), unit);
}

} //namespace



GeoValue* Database::GetGeo(const std::string& key, bool to_create) {
    Item* item = GetItem(key);
    if (item == nullptr) {
        if (!to_create) {
            return nullptr;
        }
        if (!CanReplace(key, Item{.value = GeoValue{}})) {
            status_ = Status::kOutOfMemory;
            return nullptr;
        }
        return &std::get<GeoValue>(data_[key].value);
    }
    if (!std::holds_alternative<GeoValue>(item->value)) {
        status_ = Status::kWrongType;
        return nullptr;
    }
    return &std::get<GeoValue>(item->value);
}

size_t Database::GeoAdd(const std::string& key, const std::vector<std::tuple<double, double, std::string>>& points) {
    status_ = Status::kOk;
    GeoValue* geo = GetGeo(key, true);
    if (geo == nullptr) {
        return 0;
    }

    size_t extra_memory = 0;
    for (const auto& [lat, lon, member] : points) {
        if (!geo->contains(member)) {
            extra_memory += StringMemory(member) + kMinMemoryGeoCoords;
        }
    }
    if (!CanGrow(extra_memory)) {
        status_ = Status::kOutOfMemory;
        return 0;
    }

    size_t count = 0;
    for (const auto& [lat, lon, member] : points) {
        if (!geo->contains(member)) {
            geo->emplace(member, std::tuple{lat, lon});
            ++count;
        }
        else {
            (*geo)[member] = std::tuple{lat, lon};
        }
    }
    return count;
}

std::vector<std::optional<std::tuple<double, double>>> Database::GeoPos(const std::string& key, const std::vector<std::string>& members) {
    std::vector<std::optional<std::tuple<double, double>>> res;
    res.reserve(members.size());
    status_ = Status::kOk;
    if (!CheckGeoStatus(key)) {
        for (size_t i = 0; i < members.size(); ++i) {
            res.push_back(std::nullopt);
        }
        return res;
    }

    GeoValue* geo = GetGeo(key, false);

    for (const auto& member : members) {
        auto it = geo->find(member);
        if (it == geo->end()) {
            res.push_back(std::nullopt);
        }
        else {
            const auto& [lat, lon] = it->second;
            res.push_back(std::tuple{lat, lon});
        }
    }
    return res;
}

std::optional<double> Database::GeoDist(const std::string& key, const std::string& member1, const std::string& member2, GeoUnit unit) {
    status_ = Status::kOk;
    if (!CheckGeoStatus(key)) {
        return std::nullopt;
    }

    GeoValue* geo = GetGeo(key, false);

    auto it1 = geo->find(member1);
    auto it2 = geo->find(member2);
    if (it1 == geo->end() || it2 == geo->end()) {
        return std::nullopt;
    }
    auto& [lat1, lon1] = it1->second;
    auto& [lat2, lon2] = it2->second;
    return DistFormulaUnit(lat1, lon1, lat2, lon2, unit);
}

std::vector<std::string> Database::GeoSearch(const std::string& key, const GeoSearchOptions& options) {
    status_ = Status::kOk;
    if (!CheckGeoStatus(key)) {
        return {};
    }

    GeoValue* geo = GetGeo(key, false);

    struct PlaceInfo {
        std::string member;
        double dist;

        bool operator<(const PlaceInfo& other) const {
            return this->dist < other.dist;
        }

        bool operator>(const PlaceInfo& other) const {
            return this->dist > other.dist;
        }
    };

    std::vector<PlaceInfo> infos;
    infos.reserve(geo->size());
    for (const auto& [member, coords] : *geo) {
        auto& [lat, lon] = coords;
        double dist = DistFormulaUnit(options.lat, options.lon, lat, lon, options.unit);
        if (dist <= options.radius) {
            infos.push_back({member, dist});
        }
    }

    if (options.has_asc) {
        if (options.asc) {
            std::sort(infos.begin(), infos.end());
        }
        else {
            std::sort(infos.begin(), infos.end(), std::greater<PlaceInfo>());
        }
    }

    if (options.has_count && options.count >= 0 && static_cast<size_t>(options.count) < infos.size()) {
        infos.resize(static_cast<size_t>(options.count));
    }

    std::vector<std::string> res;
    res.reserve(infos.size());
    for (const auto& info : infos) {
        res.push_back(info.member);
    }
    return res;
}

size_t Database::GeoSearchStore(const std::string& dest, const std::string& source, const GeoSearchOptions& options) {
    status_ = Status::kOk;
    if (!CheckGeoStatus(source)) {
        return 0;
    }
    
    GeoValue* src = GetGeo(source, false);
    
    const std::vector<std::string> members = GeoSearch(source, options);
    GeoValue new_geo;
    for (const auto& member : members) {
        auto it = src->find(member);
        if (it != src->end()) {
            new_geo[member] = it->second;
        }
    }

    if (!CanReplace(dest, Item{.value = std::move(new_geo)})) {
        status_ = Status::kOutOfMemory;
        return 0;
    }
    return members.size();
}