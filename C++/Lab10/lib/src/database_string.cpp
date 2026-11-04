#include "../include/database.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <variant>

void Database::Set(const std::string& key, const std::string& value) {
    status_ = Status::kOk;
    if (!CanReplace(key, Item{.value = value})) {
        status_ = Status::kOutOfMemory;
    }
}

std::string Database::Get(const std::string& key) {
    status_ = Status::kOk;
    if (!CheckStringStatus(key)) {
        return "";
    }
    const Item* item = GetItem(key);
    return std::get<StringValue>(item->value);
}

size_t Database::Strlen(const std::string& key) {
    status_ = Status::kOk;
    if (!CheckStringStatus(key)) {
        return 0;
    }
    const Item* item = GetItem(key);
    return std::get<StringValue>(item->value).size();
}

size_t Database::Append(const std::string& key, const std::string& value) {
    status_ = Status::kOk;
    std::string new_value;
    const Item* item = GetItem(key);
    if (item == nullptr) {
        new_value = value;
    }
    else {
        if (!std::holds_alternative<StringValue>(item->value)) {
            status_ = Status::kWrongType;
            return 0;
        }
        new_value = std::get<StringValue>(item->value);
        new_value += value;
    }

    size_t len = new_value.size();
    if (!CanReplace(key, Item{.value = std::move(new_value)})) {
        status_ = Status::kOutOfMemory;
        return 0;
    }
    return len;
}

long long Database::Expire(const std::string& key, size_t seconds) {
    status_ = Status::kOk;
    Item* item = GetItem(key);
    if (item == nullptr) {
        return 0;
    }
    item->expires_at = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return 1;
}

long long Database::TTL(const std::string& key) {
    status_ = Status::kOk;
    Item* item = GetItem(key);
    if (item == nullptr) {
        status_ = Status::kNotFound;
        return -2;
    }
    if (!item->expires_at) {
        return -1;
    }
    auto left = std::chrono::duration_cast<std::chrono::seconds>(*item->expires_at - std::chrono::steady_clock::now());
    if (left.count() <= 0) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            RemoveMemory(key, it->second);
            data_.erase(it);
        }
        status_ = Status::kNotFound;
        return -2;
    }
    return left.count();
}