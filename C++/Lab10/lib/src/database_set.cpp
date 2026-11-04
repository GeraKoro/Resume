#include "../include/database.h"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

SetValue* Database::GetSet(const std::string& key, bool to_create) {
    Item* item = GetItem(key);
    if (item == nullptr) {
        if (!to_create) {
            return nullptr;
        }
        if (!CanReplace(key, Item{.value = SetValue{}})) {
            status_ = Status::kOutOfMemory;
            return nullptr;
        }
        return &std::get<SetValue>(data_[key].value);
    }
    if (!std::holds_alternative<SetValue>(data_[key].value)) {
        status_ = Status::kWrongType;
        return nullptr;
    }
    return &std::get<SetValue>(item->value);
}

size_t Database::SAdd(const std::string& key, const std::vector<std::string>& members) {
    status_ = Status::kOk;
    SetValue* set = GetSet(key, true);
    if (set == nullptr) {
        return 0;
    }

    size_t extra_memory = 0;
    for (const auto& member : members) {
        if (!set->contains(member)) {
            extra_memory += StringMemory(member);
        }
    }
    if (!CanGrow(extra_memory)) {
        status_ = Status::kOutOfMemory;
        return 0;
    }

    size_t count = 0;
    for (const auto& member : members) {
        if (set->insert(member).second) {
            ++count;
        }
    }
    return count;
}

size_t Database::SRem(const std::string& key, const std::vector<std::string>& members) {
    SetValue* set = GetSet(key, false);
    if (set == nullptr) {
        return 0;
    }

    auto it = data_.find(key);
    size_t before = ItemMemory(key, it->second);
    size_t count = 0;
    for (const auto& member : members) {
        if (set->erase(member)) {
            ++count;
        }
    }
    size_t after = ItemMemory(key, it->second);
    if (after < before) {
        ReleaseMemory(before - after);
    }
    return count;
}

bool Database::SIsMember(const std::string& key, const std::string& member) {
    status_ = Status::kOk;
    if (!CheckSetStatus(key)) {
        return false;
    }
    SetValue* set = GetSet(key, false);
    return set->contains(member);
}

std::vector<std::string> Database::SMembers(const std::string& key) {
    status_ = Status::kOk;
    std::vector<std::string> res;
    if (!CheckSetStatus(key)) {
        return res;
    }
    SetValue* set = GetSet(key, false);

    for (const auto& member : *set) {
        res.push_back(member);
    }
    return res;
}

size_t Database::SCard(const std::string& key) {
    status_ = Status::kOk;
    if (!CheckSetStatus(key)) {
        return 0;
    }
    SetValue* set = GetSet(key, false);
    return set->size();
}

std::vector<std::string> Database::SUnion(const std::vector<std::string>& keys) {
    status_ = Status::kOk;
    if (keys.empty()) {
        return {};
    }

    std::unordered_set<std::string> result;
    for (const auto& key : keys) {
        const Item* item = GetItem(key);
        if (item == nullptr) {
            continue;
        }
        if (!std::holds_alternative<SetValue>(item->value)) {
            status_ = Status::kWrongType;
            return {};
        }
        const SetValue& set = std::get<SetValue>(item->value);
        result.insert(set.begin(), set.end());
    }

    std::vector<std::string> res(result.begin(), result.end());
    return res;
}

std::vector<std::string> Database::SInter(const std::vector<std::string>& keys) {
    status_ = Status::kOk;
    if (keys.empty()) {
        return {};
    }

    for (const auto& key : keys) {
        const Item* item = GetItem(key);
        if (item == nullptr) {
            return {};
        }
        if (!std::holds_alternative<SetValue>(item->value)) {
            status_ = Status::kWrongType;
            return {};
        }
    }

    const SetValue& first_set = std::get<SetValue>(GetItem(keys[0])->value);
    std::unordered_set<std::string> result(first_set.begin(), first_set.end());
    for (size_t i = 1; i < keys.size(); ++i) {
        const SetValue& set = std::get<SetValue>(GetItem(keys[i])->value);
        std::unordered_set<std::string> inter_set;
        for (const auto& member : result) {
            if (set.contains(member)) {
                inter_set.insert(member);
            }
        }
        result = std::move(inter_set);
    }

    std::vector<std::string> res(result.begin(), result.end());
    return res;
}

std::vector<std::string> Database::SDiff(const std::vector<std::string>& keys) {
    status_ = Status::kOk;
    if (keys.empty()) {
        return {};
    }

    const Item* first_item = GetItem(keys[0]);
    if (first_item == nullptr) {
        return {};
    }
    if (!std::holds_alternative<SetValue>(first_item->value)) {
        status_ = Status::kWrongType;
        return {};
    }

    std::unordered_set<std::string> result(std::get<SetValue>(first_item->value).begin(), std::get<SetValue>(first_item->value).end());
    for (size_t i = 1; i < keys.size(); ++i) {
        const Item* item = GetItem(keys[i]);
        if (item == nullptr) {
            continue;
        }
        if (!std::holds_alternative<SetValue>(item->value)) {
            status_ = Status::kWrongType;
            return {};
        }
        const SetValue& set = std::get<SetValue>(item->value);
        for (const auto& member : set) {
            result.erase(member);
        }
    }
    std::vector<std::string> res(result.begin(), result.end());
    return res;
}

size_t Database::SMove(const std::string& source, const std::string& destination, const std::string& member) {
    status_ = Status::kOk;
    if (!CheckSetStatus(source)) {
        return 0;
    }
    SetValue* src = GetSet(source, false);
    if (src->size() == 0 || !src->contains(member)) {
        return 0;
    }

    SetValue* dst = GetSet(destination, true);
    if (dst == nullptr) {
        return 0;
    }
    if (!dst->contains(member) && !CanGrow(StringMemory(member))) {
        status_ = Status::kOutOfMemory;
        return 0;
    }
    src->erase(member);
    ReleaseMemory(StringMemory(member));
    dst->insert(member);
    return 1;
}