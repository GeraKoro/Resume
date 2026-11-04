#include "test_framework.h"

#include "api_work.h"
#include "app_main.h"

#include <cstddef>
#include <expected>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

class SequenceHttp final : public Http {
public:
    explicit SequenceHttp(std::vector<std::string> v) : v_(std::move(v)) {}

    std::expected<std::string, std::string> Get(const std::string& url) override {
        url_ = url;
        ++calls_;
        if (i_ >= v_.size()) {
            return std::unexpected("no more replies");
        }
        return v_[i_++];
    }

    int Calls() const { return calls_; }

private:
    std::vector<std::string> v_;
    std::size_t i_ = 0;
    int calls_ = 0;
    std::string url_;
};

TEST_CASE("appmain_smoke_exit") {
    const std::string a = R"json(
{"countries":[{"regions":[{"settlements":[{"title":"A","stations":[{"codes":{"yandex_code":"cA"},"title":"A","station_type":"train_station","transport_type":"train"}]}]}]}]}
)json";
    const std::string b = R"json(
{"countries":[{"regions":[{"settlements":[{"title":"B","stations":[{"codes":{"yandex_code":"cB"},"title":"B","station_type":"train_station","transport_type":"train"}]}]}]}]}
)json";
    const std::string p = R"json({"segments":[]})json";

    auto http = std::make_shared<SequenceHttp>(std::vector<std::string>{a, b, p});

    std::istringstream in("exit\n");
    std::ostringstream out;
    std::ostringstream err;
    const int r = AppMain(std::vector<std::string>{"routes_app", "2026-03-30", "A", "B"}, in, out, err, http);
    ASSERT_EQ(r, 0);
    ASSERT_TRUE(out.str().find("Поиск маршрутов") != std::string::npos);
}

TEST_CASE("appmain_usage_when_not_enough_args") {
    std::istringstream in("");
    std::ostringstream out;
    std::ostringstream err;
    const int r = AppMain(std::vector<std::string>{"routes_app"}, in, out, err, std::make_shared<SequenceHttp>(std::vector<std::string>{}));
    ASSERT_EQ(r, 1);
    ASSERT_TRUE(err.str().find("Usage: routes_app") != std::string::npos);
}

TEST_CASE("appmain_handles_findstation_error") {
    auto http = std::make_shared<SequenceHttp>(std::vector<std::string>{"not json"});

    std::istringstream in("");
    std::ostringstream out;
    std::ostringstream err;
    const int r = AppMain(std::vector<std::string>{"routes_app", "2026-03-30", "A", "B"}, in, out, err, http);
    ASSERT_EQ(r, 2);
    ASSERT_TRUE(err.str().find("Ошибка поиска станции отправления") != std::string::npos);
}
