#include "app_main.h"

#include "api_work.h"
#include "env_config.h"
#include "print.h"

#include <chrono>
#include <cstddef>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

int AppMain(const std::vector<std::string>& args, std::istream& in, std::ostream& out, std::ostream& err, std::shared_ptr<Http> http) {
    if (args.size() < 3) {
        const std::string prog = args.empty() ? "routes_app" : args[0];
        err << "Usage: " << prog << " <YYYY-MM-DD> <from_city> <to_city>\n";
        err << "       " << prog << " <YYYY-MM-DD> <to_city>   (from_city = Санкт-Петербург)\n";
        return 1;
    }

    const std::string date = args[1];
    Print print;
    const std::string from_city = (args.size() >= 4) ? print.Destination(args[2]) : "Санкт-Петербург";
    const std::string destination_city = (args.size() >= 4) ? print.Destination(args[3]) : print.Destination(args[2]);

    const std::string api_key = GetApiKey();
    if (api_key.empty()) {
        err << "Нет API-ключа: задайте YANDEX_API_KEY в окружении или первой строкой в .env: YANDEX_API_KEY=...\n";
        return 1;
    }
    ApiCache cache(512, std::chrono::hours(24));
    Api api(api_key, std::move(http), std::move(cache));

    std::string current_date = date;
    std::string current_from = from_city;
    std::string current_destination = destination_city;

    while (true) {
        out << "\nПоиск маршрутов из " << current_from << " в " << current_destination << " на дату " << current_date << "...\n";

        auto from_list = api.FindAllStations(current_from);
        auto to_list = api.FindAllStations(current_destination);
        if (!from_list.has_value()) {
            err << "Ошибка поиска станции отправления: " << from_list.error() << '\n';
            return 2;
        }
        if (!to_list.has_value()) {
            err << "Ошибка поиска станции назначения: " << to_list.error() << '\n';
            return 2;
        }
        if (from_list->empty() || to_list->empty()) {
            err << "Не удалось определить коды станций.\n";
            return 2;
        }

        auto found = api.FindPathsForStations(*from_list, *to_list, current_date);
        if (!found.has_value()) {
            err << "Ошибка поиска маршрутов: " << found.error() << '\n';
            return 3;
        }

        const auto routes = MakeRoutes(*found, std::vector<Path>{}, std::vector<Path>{}, current_date, current_from, current_destination);

        std::size_t shown = 5;
        print.Routes(out, routes, shown);

        while (true) {
            out << "Введите команду: [more] больше, [all] все, [next] новый запрос, [back] обратно, [exit] выход: ";
            std::string cmd;
            if (!std::getline(in >> std::ws, cmd)) {
                return 0;
            }
            if (cmd == "exit") {
                return 0;
            }
            if (cmd == "more") {
                shown += 5;
                print.Routes(out, routes, shown);
                continue;
            }
            if (cmd == "all") {
                print.Routes(out, routes, routes.size());
                continue;
            }
            if (cmd == "back") {
                auto back_found = api.FindPathsForStations(*to_list, *from_list, current_date);
                if (!back_found.has_value()) {
                    err << "Ошибка обратного поиска: " << back_found.error() << '\n';
                } else {
                    auto back_routes = MakeRoutes(*back_found, std::vector<Path>{}, std::vector<Path>{}, current_date, current_destination, current_from);
                    std::size_t back_shown = 5;
                    print.Routes(out, back_routes, back_shown);
                }
                continue;
            }
            if (cmd == "next") {
                out << "Дата (YYYY-MM-DD): ";
                std::getline(in, current_date);
                out << "Город отправления: ";
                std::getline(in, current_from);
                current_from = print.Destination(current_from);
                out << "Город назначения: ";
                std::getline(in, current_destination);
                current_destination = print.Destination(current_destination);
                break;
            }

            out << "Неизвестная команда.\n";
        }
    }

    return 0;
}
