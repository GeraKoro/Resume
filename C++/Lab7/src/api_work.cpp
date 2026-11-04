#include "api_work.h"

#include "objects.h"

#include <cpr/api.h>
#include <cpr/error.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <exception>
#include <expected>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using json = nlohmann::json;

constexpr const char* kBaseUrl = "https://api.rasp.yandex.net/v3.0";

namespace {

std::string Trim(std::string s) {
    const auto n0 = s.find_first_not_of(" \t\r\n");
    if (n0 == std::string::npos) {
        return {};
    }
    const auto n1 = s.find_last_not_of(" \t\r\n");
    return s.substr(n0, n1 - n0 + 1);
}

bool CityMatch(const std::string& settlement_title, const std::string& city) {
    const std::string a = Trim(settlement_title);
    const std::string b = Trim(city);
    if (a.empty() || b.empty()) {
        return false;
    }
    if (a == b) {
        return true;
    }
    if (a.find(b) != std::string::npos) {
        return true;
    }
    if (b.find(a) != std::string::npos) {
        return true;
    }
    return false;
}

std::vector<std::string> StationQueriesForCity(const std::string& city) {
    const std::string t = Trim(city);
    std::vector<std::string> out;
    auto push = [&](const std::string& s) {
        if (s.empty()) {
            return;
        }
        if (std::find(out.begin(), out.end(), s) == out.end()) {
            out.push_back(s);
        }
    };
    push(t);
    std::string lower;
    lower.reserve(t.size());
    for (unsigned char ch : t) {
        lower += static_cast<char>(std::tolower(ch));
    }
    if (lower == "москва" || lower == "moscow") {
        push("Moscow");
        push("Москва");
    }
    if (lower == "пхеньян" || lower == "pyongyang") {
        push("Pyongyang");
        push("Пхеньян");
    }
    if (lower == "могадишо" || lower == "mogadishu" || lower == "сомали" || lower == "somalia") {
        push("Mogadishu");
        push("Могадишо");
    }
    return out;
}

bool SettlementMatchesUserCity(const std::string& settlement_title, const std::string& user_city) {
    for (const auto& q : StationQueriesForCity(user_city)) {
        if (CityMatch(settlement_title, q)) {
            return true;
        }
    }
    return false;
}

std::optional<Station> PickTrainStation(const std::vector<Station>& stations) {
    for (const auto& station : stations) {
        if (station.GetTransportType() == "train" && station.GetType() == "train_station") {
            return station;
        }
    }
    for (const auto& station : stations) {
        if (station.GetTransportType() == "train") {
            return station;
        }
    }
    return std::nullopt;
}

std::optional<Station> PickPlaneStation(const std::vector<Station>& stations) {
    for (const auto& station : stations) {
        if (station.GetTransportType() == "plane" && station.GetType() == "airport") {
            return station;
        }
    }
    for (const auto& station : stations) {
        if (station.GetTransportType() == "plane") {
            return station;
        }
    }
    for (const auto& station : stations) {
        if (station.GetType() == "airport") {
            return station;
        }
    }
    return std::nullopt;
}

Station PickStation(const std::vector<Station>& stations) {
    if (const auto t = PickTrainStation(stations)) {
        return *t;
    }
    if (const auto p = PickPlaneStation(stations)) {
        return *p;
    }
    return stations.front();
}

json GetEndpoint(const json& segment, bool departure_end) {
    if (departure_end) {
        if (segment.contains("departure_from")) {
            return segment["departure_from"];
        }
        return segment.value("from", json::object());
    }
    if (segment.contains("arrival_to")) {
        return segment["arrival_to"];
    }
    return segment.value("to", json::object());
}

std::string GetTransportTypes(const json& segment) {
    if (const auto it = segment.find("transport_types"); it != segment.end() && it->is_array()) {
        std::vector<std::string> uniq;
        for (const auto& t : *it) {
            if (!t.is_string()) {
                continue;
            }
            const auto s = t.get<std::string>();
            if (std::find(uniq.begin(), uniq.end(), s) == uniq.end()) {
                uniq.push_back(s);
            }
        }
        if (!uniq.empty()) {
            std::ostringstream out;
            for (std::size_t i = 0; i < uniq.size(); ++i) {
                if (i != 0) {
                    out << ", ";
                }
                out << uniq[i];
            }
            return out.str();
        }
    }
    if (const auto th = segment.value("thread", json::object()); !th.empty()) {
        return th.value("transport_type", "unknown");
    }
    return "unknown";
}

std::string GetThreadNumbers(const json& segment) {
    if (const auto it = segment.find("details"); it != segment.end() && it->is_array()) {
        std::string out;
        for (const auto& detail : *it) {
            if (!detail.contains("thread") || !detail["thread"].is_object()) {
                continue;
            }
            const auto num = detail["thread"].value("number", std::string{});
            if (num.empty()) {
                continue;
            }
            if (!out.empty()) {
                out += " / ";
            }
            out += num;
        }
        if (!out.empty()) {
            return out;
        }
    }
    if (const auto th = segment.value("thread", json::object()); !th.empty()) {
        return th.value("number", std::string{"unknown"});
    }
    return "unknown";
}

int GetDurationSec(const json& segment) {
    if (const auto it = segment.find("duration"); it != segment.end() && it->is_number()) {
        return static_cast<int>(it->get<double>());
    }
    double sum = 0;
    if (const auto it = segment.find("details"); it != segment.end() && it->is_array()) {
        for (const auto& detail : *it) {
            const auto d = detail.find("duration");
            if (d != detail.end() && d->is_number()) {
                sum += d->get<double>();
            }
        }
    }
    return static_cast<int>(sum);
}

std::vector<Path> GetConnection(const json& segment) {
    std::vector<Path> legs;
    const auto it = segment.find("details");
    if (it == segment.end() || !it->is_array()) {
        return legs;
    }
    std::string fallback_type = GetTransportTypes(segment);
    for (const auto& detail : *it) {
        json from = detail.value("from", json::object());
        json to = detail.value("to", json::object());
        if (detail.contains("departure_from") && detail["departure_from"].is_object()) {
            from = detail["departure_from"];
        }
        if (detail.contains("arrival_to") && detail["arrival_to"].is_object()) {
            to = detail["arrival_to"];
        }
        const std::string from_name = from.value("title", "");
        const std::string from_code = from.value("code", "");
        const std::string to_name = to.value("title", "");
        const std::string to_code = to.value("code", "");
        const std::string dep = detail.value("departure", "");
        const std::string arr = detail.value("arrival", "");
        if (from_name.empty() || to_name.empty() || dep.empty() || arr.empty()) {
            continue;
        }
        std::string ttype = fallback_type;
        std::string tname = "?";
        if (detail.contains("thread") && detail["thread"].is_object()) {
            const auto& th = detail["thread"];
            ttype = th.value("transport_type", ttype);
            tname = th.value("number", tname);
        }
        int dur_sec = 0;
        if (const auto d = detail.find("duration"); d != detail.end() && d->is_number()) {
            dur_sec = static_cast<int>(d->get<double>());
        }
        const std::size_t dur_min = dur_sec > 0 ? static_cast<std::size_t>(dur_sec / 60) : 0U;
        legs.emplace_back(ttype, tname, from_name, from_code, to_name, to_code, dep, arr, dur_min);
    }
    return legs;
}

} // namespace

std::string BuildStationsUrl(const std::string& api_key, const std::string& city) {
    std::ostringstream out;
    out << kBaseUrl << "/stations_list/?apikey=" << api_key << "&lang=ru_RU&format=json";
    out << "&title=" << city;
    return out.str();
}

std::string BuildSearchUrl(const std::string& api_key, const std::string& from_code, const std::string& to_code, const std::string& date) {
    std::ostringstream out;
    out << kBaseUrl << "/search/?apikey=" << api_key << "&from=" << from_code << "&to=" << to_code << "&date=" << date
        << "&transfers=true&lang=ru_RU&format=json";
    return out.str();
}

std::expected<std::string, std::string> CprHttp::Get(const std::string& url) {
    const auto response = cpr::Get(cpr::Url{url});
    if (response.error.code != cpr::ErrorCode::OK) {
        return std::unexpected("HTTP error: " + response.error.message);
    }
    if (response.status_code != 200) {
        return std::unexpected("HTTP status code: " + std::to_string(response.status_code));
    }
    return response.text;
}

Api::Api(std::string api_key, std::shared_ptr<Http> http, ApiCache cache)
    : api_key_(std::move(api_key)), http_(std::move(http)), cache_(std::move(cache)) {}

std::expected<std::vector<Station>, std::string> Api::FindAllStations(const std::string& city) {
    std::vector<Station> result;
    std::unordered_set<std::string> seen_codes;
    try {
        for (const auto& query : StationQueriesForCity(city)) {
            const auto key = "stations:" + query;
            const auto url = BuildStationsUrl(api_key_, UrlEncode(query));
            auto text = GetCached(key, url);
            if (!text.has_value()) {
                return std::unexpected(text.error());
            }
            const auto data = json::parse(*text);
            if (!data.contains("countries")) {
                continue;
            }
            for (const auto& country : data["countries"]) {
                if (!country.contains("regions")) {
                    continue;
                }
                for (const auto& region : country["regions"]) {
                    if (!region.contains("settlements")) {
                        continue;
                    }
                    for (const auto& settlement : region["settlements"]) {
                        const auto settlement_title = settlement.value("title", "");
                        if (!SettlementMatchesUserCity(settlement_title, city)) {
                            continue;
                        }
                        if (!settlement.contains("stations")) {
                            continue;
                        }
                        for (const auto& station : settlement["stations"]) {
                            const std::string code = station.value("codes", json::object()).value("yandex_code", "");
                            if (code.empty() || !seen_codes.insert(code).second) {
                                continue;
                            }
                            result.emplace_back(code, station.value("title", ""), station.value("station_type", ""), settlement_title,
                                                station.value("transport_type", ""));
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& except) {
        return std::unexpected(std::string("Failed to parse stations JSON: ") + except.what());
    }
    return result;
}

std::expected<std::optional<Station>, std::string> Api::FindStation(const std::string& city) {
    auto stations = FindAllStations(city);
    if (!stations.has_value()) {
        return std::unexpected(stations.error());
    }
    if (stations->empty()) {
        return std::optional<Station>{};
    }
    return std::optional<Station>{PickStation(*stations)};
}

std::expected<std::vector<Path>, std::string> Api::FindPaths(const std::string& from_code, const std::string& to_code, const std::string& date) {
    const auto key = "paths:" + from_code + "->" + to_code + ":" + date;
    const auto url = BuildSearchUrl(api_key_, UrlEncode(from_code), UrlEncode(to_code), UrlEncode(date));
    auto text = GetCached(key, url);
    if (!text.has_value()) {
        return std::unexpected(text.error());
    }

    std::vector<Path> paths;
    try {
        const auto data = json::parse(*text);
        if (!data.contains("segments")) {
            return paths;
        }
        for (const auto& segment : data["segments"]) {
            const auto from = GetEndpoint(segment, true);
            const auto to = GetEndpoint(segment, false);

            const std::string transport_type = GetTransportTypes(segment);
            const std::string transport_name = GetThreadNumbers(segment);
            const std::string from_name = from.value("title", "");
            const std::string from_station_code = from.value("code", "");
            const std::string to_name = to.value("title", "");
            const std::string to_station_code = to.value("code", "");
            const std::string departure_time = segment.value("departure", "");
            const std::string arrival_time = segment.value("arrival", "");
            const int duration_sec = GetDurationSec(segment);
            const std::size_t duration_min = duration_sec > 0 ? static_cast<std::size_t>(duration_sec / 60) : 0U;

            Path path(transport_type, transport_name, from_name, from_station_code, to_name, to_station_code, departure_time, arrival_time, duration_min);
            auto connection = GetConnection(segment);
            if (connection.size() >= 2) {
                path.SetConnection(std::move(connection));
            }
            paths.push_back(std::move(path));
        }
    }
    catch (const std::exception& except) {
        return std::unexpected(std::string("Failed to parse paths: ") + except.what());
    }
    return paths;
}

std::expected<std::vector<Path>, std::string> Api::FindPathsForStations(const std::vector<Station>& from_stations, const std::vector<Station>& to_stations, const std::string& date) {
    std::vector<Path> merged;
    const auto tf = PickTrainStation(from_stations);
    const auto tt = PickTrainStation(to_stations);
    const auto pf = PickPlaneStation(from_stations);
    const auto pt = PickPlaneStation(to_stations);

    if (tf.has_value() && tt.has_value()) {
        auto p = FindPaths(tf->GetCode(), tt->GetCode(), date);
        if (!p.has_value()) {
            return std::unexpected(p.error());
        }
        merged.insert(merged.end(), p->begin(), p->end());
    }
    if (pf.has_value() && pt.has_value()) {
        const bool dup = tf.has_value() && tt.has_value() && pf->GetCode() == tf->GetCode() && pt->GetCode() == tt->GetCode();
        if (!dup) {
            auto p = FindPaths(pf->GetCode(), pt->GetCode(), date);
            if (!p.has_value()) {
                return std::unexpected(p.error());
            }
            merged.insert(merged.end(), p->begin(), p->end());
        }
    }
    return merged;
}

std::expected<std::string, std::string> Api::GetCached(const std::string& key, const std::string& url) {
    if (const auto from_cache = cache_.Get(key); from_cache.has_value()) {
        return *from_cache;
    }

    auto response = http_->Get(url);
    if (!response.has_value()) {
        return std::unexpected(response.error());
    }

    cache_.Put(key, *response);
    return *response;
}

std::string Api::UrlEncode(const std::string& value) {
    std::string out;
    out.reserve(value.size() * 3);
    constexpr char k_hex_digits[] = "0123456789ABCDEF";
    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<char>(c);
            continue;
        }
        out += '%';
        out += k_hex_digits[(c >> 4) & 0xF];
        out += k_hex_digits[c & 0xF];
    }
    return out;
}

std::vector<Route> MakeRoutes(const std::vector<Path>& direct, const std::vector<Path>& from_city, const std::vector<Path>& to_city, const std::string& date,
                              const std::string& from_city_name, const std::string& to_city_name)
{
    constexpr std::size_t kMaxTransferRoutes = 300;

    auto is_transfer_feasible = [](const Path& first_leg, const Path& second_leg) {
        const auto& first_arrival = first_leg.GetArrivalTime();
        const auto& second_departure = second_leg.GetDepartureTime();
        return !first_arrival.empty() && !second_departure.empty() && second_departure >= first_arrival;
    };

    std::vector<Route> result;
    result.reserve(direct.size() + std::min(from_city.size(), kMaxTransferRoutes));
    for (const auto& d : direct) {
        result.emplace_back(d, date, from_city_name, to_city_name);
    }

    std::unordered_map<std::string, std::vector<Path>> arrivals_to_city;
    for (const auto& second : to_city) {
        arrivals_to_city[second.GetDepartureStationCode()].push_back(second);
    }

    for (const auto& first : from_city) {
        auto hub_it = arrivals_to_city.find(first.GetArrivalStationCode());
        if (hub_it == arrivals_to_city.end()) {
            continue;
        }

        for (const auto& second : hub_it->second) {
            if (!is_transfer_feasible(first, second)) {
                continue;
            }
            result.emplace_back(first, second, date, from_city_name, to_city_name);
            if (result.size() >= direct.size() + kMaxTransferRoutes) {
                break;
            }
        }
        if (result.size() >= direct.size() + kMaxTransferRoutes) {
            break;
        }
    }

    std::sort(result.begin(), result.end(), [](const Route& a, const Route& b) { return a.GetTotalDuration() < b.GetTotalDuration(); });
    return result;
}
