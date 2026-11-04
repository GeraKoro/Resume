#pragma once

#include "cache.h"
#include "objects.h"

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Http {
public:
    virtual ~Http() = default;
    virtual std::expected<std::string, std::string> Get(const std::string& url) = 0;
};

class CprHttp final : public Http {
public:
    std::expected<std::string, std::string> Get(const std::string& url) override;
};

class Api {
public:
    Api(std::string api_key, std::shared_ptr<Http> http, ApiCache cache);

    std::expected<std::vector<Station>, std::string> FindAllStations(const std::string& city);
    std::expected<std::optional<Station>, std::string> FindStation(const std::string& city);

    std::expected<std::vector<Path>, std::string> FindPaths(const std::string& from_code, const std::string& to_code, const std::string& date);

    std::expected<std::vector<Path>, std::string> FindPathsForStations(const std::vector<Station>& from_stations, const std::vector<Station>& to_stations, const std::string& date);

private:
    std::expected<std::string, std::string> GetCached(const std::string& key, const std::string& url);
    std::string UrlEncode(const std::string& value);
    std::string api_key_;
    std::shared_ptr<Http> http_;
    ApiCache cache_;
};

std::vector<Route> MakeRoutes(const std::vector<Path>& direct, const std::vector<Path>& from_spb, const std::vector<Path>& to_city, const std::string& date,
                              const std::string& from_city_name, const std::string& to_city_name);
