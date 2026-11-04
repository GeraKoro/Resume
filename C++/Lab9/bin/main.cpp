#include "../app/location.h"
#include "../app/weather.h"
#include "../app/places.h"
#include "../app/route.h"
#include "../lib/ttask.h"

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

int main() {
    try {
        TTaskScheduler scheduler;
        auto location_task = scheduler.add([]() {
            std::cout << "Ищем ваше местоположение: \n";
            return geo::ApiWorkLocation();
        });

        auto weather_task = scheduler.add([](geo::Point pos) {
            std::cout << "\nПогода:\n";
            weather::CurrentWeather w = weather::GetWeather(pos);
            std::cout << "Температура: " << w.temperature << "°C" << " (ощущается как " << w.apparent_temperature << "°C)\n";
            std::cout << "Ветер: " << w.wind_speed << " м/с\n";
            std::cout << "Осадки: " << w.precipitation_mm << " мм\n";
            if (!w.warning.message.empty()) {
                std::cout << "Предупреждение: " << w.warning.message << "\n";
            }
            return w;
        }, location_task.getFutureResult<const geo::Point&>());

        auto places_task = scheduler.add([](geo::Point pos, weather::CurrentWeather w) {
            std::cout << "\nПоиск мест рядом с вами:\n";
            return place::GetPlaces(w, pos);
        }, location_task.getFutureResult<const geo::Point&>(), weather_task.getFutureResult());

        scheduler.executeAll();

        auto places = places_task.getFutureResult<std::vector<place::Place>>().get();

        if (places.empty()) {
            std::cout << "Мест не найдено\n";
            return 0;
        }

        std::cout << "\nНайденные места:\n";
        for (size_t i = 0; i < places.size() && i < 15; ++i) {
            std::cout << "  " << i + 1 << ") " << places[i].name;
            if (places[i].name != places[i].type) {
                std::cout << " (" << places[i].type << ')';
            }
            std::cout << " — " << static_cast<int>(places[i].distance_m) << " м\n";
        }

        std::cout << "\nВыберите место (номер): ";
        std::string choice_str;
        std::getline(std::cin, choice_str);
        size_t choice = std::stoul(choice_str);

        if (choice < 1 || choice > places.size() || choice > 15) {
            std::cout << "Некорректный выбор.\n";
            return 1;
        }

        const place::Place& chosen = places[choice - 1];

        std::cout << "\nМаршрут до: " << chosen.name << '\n';
        route::Route info = route::GetTime(chosen.distance_m);

        std::cout << std::fixed << std::setprecision(1);
        std::cout << "Примерное время пребытия:\n";
        std::cout << "  Пешком:       " << info.foot_time << " мин\n";
        std::cout << "  На велосипеде: " << info.bike_time << " мин\n";
        std::cout << "  На машине:    " << info.car_time << " мин\n";

        std::cout << "\nПриятного отдыха!\n";

    } 
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }

    return 0;
}