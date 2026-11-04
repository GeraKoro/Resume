#include "../include/database.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

bool Database::IsExpired(const Item& item) const {
    if (!item.expires_at) {
        return false;
    }
    return std::chrono::steady_clock::now() >= *item.expires_at;
}

void Database::RemoveIfExpired(const std::string& key) {
    auto it = data_.find(key);
    if (it != data_.end() && IsExpired(it->second)) {
        RemoveMemory(key, it->second);
        data_.erase(it);
    }
}

Item* Database::GetItem(const std::string& key) {
    RemoveIfExpired(key);
    auto it = data_.find(key);
    if (it == data_.end()) {
        return nullptr;
    }
    return &it->second;
}

const Item* Database::GetItem(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return nullptr;
    }
    if (IsExpired(it->second)) {
        return nullptr;
    }
    return &it->second;
}

void Database::ConfigSet(std::size_t bytes) {
    max_memory_ = bytes;
}

std::size_t Database::ConfigGet() const {
    return max_memory_;
}

size_t Database::MemoryUsage(const std::string& key) {
    const Item* item = GetItem(key);
    if (item == nullptr) {
        return 0;
    }
    return ItemMemory(key, *item);
}

size_t Database::Del(const std::vector<std::string>& keys) {
    size_t count = 0;
    for (const auto& key : keys) {
        auto it = data_.find(key);
        if (it == data_.end()) {
            continue;
        }
        RemoveIfExpired(key);
        it = data_.find(key);
        if (it == data_.end()) {
            continue;
        }
        RemoveMemory(key, it->second);
        data_.erase(it);
        ++count;
    }
    return count;
}

void Database::FlushDb() {
    used_memory_ = 0;
    data_.clear();
}

std::string Database::Type(const std::string& key) {
    const Item* item = GetItem(key);
    if (item == nullptr) {
        return "none";
    }
    
    return std::visit([](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, StringValue>) {
            return "string";
        }
        if constexpr (std::is_same_v<T, ListValue>) {
            return "list";
        }
        if constexpr (std::is_same_v<T, SetValue>) {
            return "set";
        }
        return "zset";
    }
    , item->value);
}

size_t Database::Exists(const std::vector<std::string>& keys) {
    size_t count = 0;
    for (const auto& key : keys) {
        if (GetItem(key) != nullptr) {
            ++count;
        }
    }
    return count;
}

size_t Database::DbSize() {
    size_t count = 0;
    for (auto it = data_.begin(); it != data_.end(); ) {
        if (IsExpired(it->second)) {
            RemoveMemory(it->first, it->second);
            it = data_.erase(it);
        }
        else {
            ++count;
            ++it;
        }
    }
    return count;
}

bool Database::PatternMatches(const std::string& key, const std::string& pattern) const {
    size_t key_i = 0;
    size_t pat_i = 0;
    size_t star_pat = std::string::npos;
    size_t star_key = std::string::npos;

    while (key_i < key.size()) {
        if (pat_i < pattern.size() && (pattern[pat_i] == key[key_i] || pattern[pat_i] == '?')) {
            ++key_i;
            ++pat_i;
        }
        else if (pat_i < pattern.size() && pattern[pat_i] == '*') {
            star_pat = pat_i++;
            star_key = key_i;
        }
        else if (star_pat != std::string::npos) {
            pat_i = star_pat + 1;
            ++star_key;
            key_i = star_key;
        }
        else {
            return false;
        }
    }

    while (pat_i < pattern.size() && pattern[pat_i] == '*') {
        ++pat_i;
    }
    return pat_i == pattern.size();
}

std::vector<std::string> Database::Keys(const std::string& pattern) {
    std::vector<std::string> result;

    for (auto it = data_.begin(); it != data_.end(); ) {
        if (IsExpired(it->second)) {
            RemoveMemory(it->first, it->second);
            it = data_.erase(it);
            continue;
        }
        if (PatternMatches(it->first, pattern)) {
            result.push_back(it->first);
        }
        ++it;
    }
    return result;
}

bool Database::CheckStringStatus(const std::string& key) {
    const Item* item = GetItem(key);
    if (item == nullptr) {
        status_ = Status::kNotFound;
        return false;
    }
    if (!std::holds_alternative<StringValue>(item->value)) {
        status_ = Status::kWrongType;
        return false;
    }
    return true;
}

bool Database::CheckListStatus(const std::string& key) {
    const Item* item = GetItem(key);
    if (item == nullptr) {
        status_ = Status::kNotFound;
        return false;
    }
    if (!std::holds_alternative<ListValue>(item->value)) {
        status_ = Status::kWrongType;
        return false;
    }
    return true;
}

bool Database::CheckSetStatus(const std::string& key) {
    const Item* item = GetItem(key);
    if (item == nullptr) {
        status_ = Status::kNotFound;
        return false;
    }
    if (!std::holds_alternative<SetValue>(item->value)) {
        status_ = Status::kWrongType;
        return false;
    }
    return true;
}

bool Database::CheckGeoStatus(const std::string& key) {
    const Item* item = GetItem(key);
    if (item == nullptr) {
        status_ = Status::kNotFound;
        return false;
    }
    if (!std::holds_alternative<GeoValue>(item->value)) {
        status_ = Status::kWrongType;
        return false;
    }
    return true;
}

// void Database::Export(std::string path) {
//     std::ofstream file;
//     file.open(path);
//     for (auto it = data_.begin(); it != data_.end(); ++it) {
//         Value item = *it->second.value;
//         if (std::holds_alternative<StringValue>(item)) {
//             file << std::get<StringValue>(item) << '\n';
//         }
//         else if (std::holds_alternative<ListValue>(item)) {
//             for (auto it_list = std::get<ListValue>(item).begin(); it_list != std::get<ListValue>(item).end(); ++it) {
//                 file << *it_list << '\n';
//             }
//         }
//         else if (std::holds_alternative<SetValue>(item)) {
//             for (auto it_set = std::get<SetValue>(item).begin(); it_set != std::get<SetValue>(item).end(); ++it) {
//                 file << *it_set << '\n';
//             }
//         }
//         else if (std::holds_alternative<GeoValue>(item)) {
//             for (auto it_geo = std::get<GeoValue>(item).begin(); it_geo != std::get<GeoValue>(item).end(); ++it) {
//                 file << *it_geo->first;
//                 auto [lat, lon] = *it_geo->second;
//                 file << lat << " " << lon << '\n';
//             }
//         }
//     }
//     file.close();
// }