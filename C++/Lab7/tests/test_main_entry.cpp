#include "test_framework.h"

#include "api_work.h"
#include "main_entry.h"

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

    std::expected<std::string, std::string> Get(const std::string&) override {
        if (i_ >= v_.size()) {
            return std::unexpected("no more replies");
        }
        return v_[i_++];
    }

private:
    std::vector<std::string> v_;
    std::size_t i_ = 0;
};

TEST_CASE("mainentry_smoke") {
    const std::string a = R"json(
{"countries":[{"regions":[{"settlements":[{"title":"A","stations":[{"codes":{"yandex_code":"cA"},"title":"A","station_type":"train_station","transport_type":"train"}]}]}]}]}
)json";
    const std::string b = R"json(
{"countries":[{"regions":[{"settlements":[{"title":"B","stations":[{"codes":{"yandex_code":"cB"},"title":"B","station_type":"train_station","transport_type":"train"}]}]}]}]}
)json";
    const std::string p = R"json({"segments":[]})json";

    auto http = std::make_shared<SequenceHttp>(std::vector<std::string>{a, b, p});

    char prog[] = "routes_app";
    char date[] = "2026-03-30";
    char from[] = "A";
    char to[] = "B";
    char* argv[] = {prog, date, from, to, nullptr};

    std::istringstream in("exit\n");
    std::ostringstream out;
    std::ostringstream err;
    const int r = MainEntry(4, argv, in, out, err, http);
    ASSERT_EQ(r, 0);
}
