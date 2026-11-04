#include "main_entry.h"

#include "api_work.h"
#include "app_main.h"

#include <cstddef>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

int MainEntry(int argc, char* argv[], std::istream& in, std::ostream& out, std::ostream& err, std::shared_ptr<Http> http) {
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i] != nullptr ? argv[i] : "");
    }
    return AppMain(std::move(args), in, out, err, std::move(http));
}
