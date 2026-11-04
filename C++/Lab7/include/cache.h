#pragma once

#include <chrono>
#include <cstddef>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>

class ApiCache {

public:
    using Clock = std::chrono::steady_clock;

private:

    struct Entry {
        std::string key;
        std::string value;
        Clock::time_point when_created;
    };

    void Update(std::list<Entry>::iterator it);
    void ClearExpired();
    void ClearOverflow();

    std::size_t max_items_;
    std::chrono::milliseconds time_alive_;
    std::list<Entry> lru_;
    std::unordered_map<std::string, std::list<Entry>::iterator> index_;

public:

    explicit ApiCache(std::size_t max_items, std::chrono::milliseconds time_alive) : max_items_(max_items), time_alive_(time_alive) {}

    std::optional<std::string> Get(const std::string& key);
    void Put(std::string key, std::string value);
};
