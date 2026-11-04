#include "../include/database.h"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

size_t Database::StringMemory(const StringValue& string) const {
    return kMinMemory + string.capacity();
}

size_t Database::ListMemory(const ListValue& list) const {
    size_t total_memory = kMinMemory;
    for (const auto& s : list) {
        total_memory += StringMemory(s);
    }
    return total_memory;
}

size_t Database::SetMemory(const SetValue& set) const {
    size_t total_memory = kMinMemory;
    for (const auto& s : set) {
        total_memory += StringMemory(s);
    }
    return total_memory;
}

size_t Database::GeoMemory(const GeoValue& geo) const {
    size_t total_memory = kMinMemory;
    for (const auto& [member, coords] : geo) {
        (void)coords;
        total_memory += StringMemory(member) + kMinMemoryGeoCoords;
    }
    return total_memory;
}

size_t Database::ItemMemory(const std::string& key, const Item& item) const {
    size_t total_memory = kMinMemory + key.capacity();
    total_memory += std::visit([this](const auto& value) -> size_t {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, StringValue>) {
            return StringMemory(value);
        }
        else if constexpr (std::is_same_v<T, ListValue>) {
            return ListMemory(value);
        }
        else if constexpr (std::is_same_v<T, SetValue>) {
            return SetMemory(value);
        }
        else {
            return GeoMemory(value);
        }
    }
    , item.value);
    return total_memory;
}

bool Database::CanFit(size_t old_bytes, size_t new_bytes) const {
    if (max_memory_ == 0) {
        return true;
    }
    if (new_bytes > max_memory_) {
        return false;
    }
    size_t new_memory = used_memory_ - old_bytes + new_bytes;
    return new_memory <= max_memory_;
}

void Database::ReleaseMemory(size_t bytes) {
    if (bytes >= used_memory_) {
        used_memory_ = 0;
    }
    else {
        used_memory_ -= bytes;
    }
}

void Database::RemoveMemory(const std::string& key, const Item& item) {
    size_t bytes = ItemMemory(key, item);
    ReleaseMemory(bytes);
}

bool Database::CanReplace(const std::string& key, Item item) {
    size_t old_bytes = 0;
    auto it = data_.find(key);
    if (it != data_.end()) {
        old_bytes = ItemMemory(key, it->second);
    }
    size_t new_bytes = ItemMemory(key, item);
    if (!CanFit(old_bytes, new_bytes)) {
        oom_ = true;
        return false;
    }
    oom_ = false;
    used_memory_ -= old_bytes;
    data_[key] = std::move(item);
    used_memory_ += new_bytes;
    return true;
}

bool Database::CanGrow(size_t extra_bytes) {
    if (extra_bytes == 0) {
        oom_ = false;
        return true;
    }
    if (max_memory_ == 0) {
        used_memory_ += extra_bytes;
        oom_ = false;
        return true;
    }
    if (used_memory_ + extra_bytes > max_memory_) {
        oom_ = true;
        return false;
    }
    used_memory_ += extra_bytes;
    oom_ = false;
    return true;
}