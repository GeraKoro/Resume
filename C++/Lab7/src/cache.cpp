#include "cache.h"

#include <iterator>
#include <list>
#include <optional>
#include <string>
#include <utility>

void ApiCache::ClearExpired() {
    const auto now = Clock::now();
    for (auto it = lru_.begin(); it != lru_.end();) {
        if (now - it->when_created > time_alive_) {
            index_.erase(it->key);
            it = lru_.erase(it);
            
        } else {
            ++it;
        }
    }
}

void ApiCache::ClearOverflow() {
    while (lru_.size() > max_items_) {
        auto last = std::prev(lru_.end());
        index_.erase(last->key);
        lru_.erase(last);
    }
}

std::optional<std::string> ApiCache::Get(const std::string& key) {
    ClearExpired();
    auto it = index_.find(key);
    if (it == index_.end()) {
        return std::nullopt;
    }
    Update(it->second);
    return it->second->value;
}

void ApiCache::Put(std::string key, std::string value) {
    ClearExpired();

    auto it = index_.find(key);
    if (it != index_.end()) {
        it->second->value = std::move(value);
        it->second->when_created = Clock::now();
        Update(it->second);
        return;
    }

    lru_.push_front(Entry{std::move(key), std::move(value), Clock::now()});
    index_[lru_.front().key] = lru_.begin();
    ClearOverflow();
}

void ApiCache::Update(std::list<Entry>::iterator it) {
    if (it == lru_.begin()) {
        return;
    }
    Entry tmp = std::move(*it);
    lru_.erase(it);
    lru_.push_front(std::move(tmp));
    index_[lru_.front().key] = lru_.begin();
}

