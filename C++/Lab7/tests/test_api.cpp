#include "test_framework.h"

#include "api_work.h"
#include "cache.h"

#include <chrono>
#include <cstddef>
#include <expected>
#include <memory>
#include <string>
#include <utility>

class FakeHttp final : public Http {
public:
    explicit FakeHttp(std::string text) : text_(std::move(text)) {}

    std::expected<std::string, std::string> Get(const std::string& url) override {
        ++calls_;
        url_ = url;
        return text_;
    }

    int Calls() const { return calls_; }
    const std::string& Url() const { return url_; }

private:
    std::string text_;
    int calls_ = 0;
    std::string url_;
};

TEST_CASE("api_findstation_prefers_train_station") {
    const std::string stations_json = R"json(
{
  "countries": [
    {
      "regions": [
        {
          "settlements": [
            {
              "title": "Город",
              "stations": [
                {
                  "codes": { "yandex_code": "s_bus" },
                  "title": "Bus stop",
                  "station_type": "bus_station",
                  "transport_type": "bus"
                },
                {
                  "codes": { "yandex_code": "s_train" },
                  "title": "Main station",
                  "station_type": "train_station",
                  "transport_type": "train"
                }
              ]
            }
          ]
        }
      ]
    }
  ]
}
)json";

    auto http = std::make_shared<FakeHttp>(stations_json);
    ApiCache cache(32, std::chrono::minutes(10));
    Api api("KEY", http, std::move(cache));

    auto st = api.FindStation("Город");
    ASSERT_TRUE(st.has_value());
    ASSERT_TRUE(st->has_value());
    ASSERT_EQ(st->value().GetCode(), std::string("s_train"));
    ASSERT_EQ(st->value().GetTransportType(), std::string("train"));
}

TEST_CASE("api_findpaths_supports_details_transport_types") {
    const std::string paths_json = R"json(
{
  "segments": [
    {
      "departure_from": { "title": "A", "code": "cA" },
      "arrival_to":   { "title": "B", "code": "cB" },
      "transport_types": ["train", "train", "plane"],
      "details": [
        { "thread": { "number": "100" }, "duration": 60,
          "from": { "title": "A", "code": "cA" }, "to": { "title": "X", "code": "cX" },
          "departure": "2026-03-30T10:00:00+03:00", "arrival": "2026-03-30T11:00:00+03:00"
        },
        { "thread": { "number": "200" }, "duration": 120,
          "from": { "title": "X", "code": "cX" }, "to": { "title": "B", "code": "cB" },
          "departure": "2026-03-30T11:30:00+03:00", "arrival": "2026-03-30T13:30:00+03:00"
        }
      ],
      "departure": "2026-03-30T10:00:00+03:00",
      "arrival": "2026-03-30T13:00:00+03:00"
    }
  ]
}
)json";

    auto http = std::make_shared<FakeHttp>(paths_json);
    ApiCache cache(32, std::chrono::minutes(10));
    Api api("KEY", http, std::move(cache));

    auto paths = api.FindPaths("cA", "cB", "2026-03-30");
    ASSERT_TRUE(paths.has_value());
    ASSERT_EQ(paths->size(), static_cast<std::size_t>(1));
    const auto& p = paths->front();
    ASSERT_TRUE(p.GetTransportType().find("train") != std::string::npos);
    ASSERT_TRUE(p.GetTransportType().find("plane") != std::string::npos);
    ASSERT_TRUE(p.GetTransportName().find("100") != std::string::npos);
    ASSERT_TRUE(p.GetTransportName().find("200") != std::string::npos);
    ASSERT_EQ(p.GetDuration(), static_cast<std::size_t>(3));
    ASSERT_EQ(p.GetConnection().size(), static_cast<std::size_t>(2));
}

TEST_CASE("api_findpaths_parses_segments_minimal") {
    const std::string paths_json = R"json(
{
  "segments": [
    {
      "from": { "title": "A", "code": "cA" },
      "to":   { "title": "B", "code": "cB" },
      "thread": { "transport_type": "train", "number": "777" },
      "departure": "2026-03-30T10:00:00+03:00",
      "arrival": "2026-03-30T12:00:00+03:00",
      "duration": 7200
    }
  ]
}
 )json";

    auto http = std::make_shared<FakeHttp>(paths_json);
    ApiCache cache(32, std::chrono::minutes(10));
    Api api("KEY", http, std::move(cache));

    auto paths = api.FindPaths("cA", "cB", "2026-03-30");
    ASSERT_TRUE(paths.has_value());
    ASSERT_EQ(paths->size(), static_cast<std::size_t>(1));

    const auto& p = paths->front();
    ASSERT_EQ(p.GetDepartureStationCode(), std::string("cA"));
    ASSERT_EQ(p.GetArrivalStationCode(), std::string("cB"));
    ASSERT_EQ(p.GetTransportType(), std::string("train"));
    ASSERT_EQ(p.GetTransportName(), std::string("777"));
    ASSERT_EQ(p.GetDuration(), static_cast<std::size_t>(120));
}

TEST_CASE("api_caches_same_request") {
    const std::string paths_json = R"json({"segments":[]})json";
    auto http = std::make_shared<FakeHttp>(paths_json);
    ApiCache cache(32, std::chrono::minutes(10));
    Api api("KEY", http, std::move(cache));

    auto a = api.FindPaths("cA", "cB", "2026-03-30");
    auto b = api.FindPaths("cA", "cB", "2026-03-30");
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    ASSERT_EQ(http->Calls(), 1);
}
