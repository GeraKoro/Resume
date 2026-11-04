#include "../include/database.h"

#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

long long Database::NormalizeInd(long long index, long long size) {
    if (index < 0) {
        index += size;
    }
    return index;
}

ListValue* Database::GetList(const std::string& key, bool to_create) {
    Item* item = GetItem(key);
    if (item == nullptr) {
        if (!to_create) {
            return nullptr;
        }
        if (!CanReplace(key, Item{.value = ListValue{}})) {
            status_ = Status::kOutOfMemory;
            return nullptr;
        }
        return &std::get<ListValue>(data_[key].value);
    }
    if (!std::holds_alternative<ListValue>(data_[key].value)) {
        status_ = Status::kWrongType;
        return nullptr;
    }
    return &std::get<ListValue>(item->value);
}

size_t Database::LPush(const std::string& key, const std::vector<std::string>& values) {
    status_ = Status::kOk;
    ListValue* list = GetList(key, true);
    if (list == nullptr) {
        return 0;
    }

    size_t extra_memory = 0;
    for (const auto& value : values) {
        extra_memory += StringMemory(value);
    }
    if (!CanGrow(extra_memory)) {
        status_ = Status::kOutOfMemory;
        return 0;
    }

    for (const auto& value : values) {
        list->push_front(value);
    }
    return list->size();
}

size_t Database::RPush(const std::string& key, const std::vector<std::string>& values) {
    status_ = Status::kOk;
    ListValue* list = GetList(key, true);
    if (list == nullptr) {
        return 0;
    }

    size_t extra_memory = 0;
    for (const auto& value : values) {
        extra_memory += StringMemory(value);
    }
    if (!CanGrow(extra_memory)) {
        status_ = Status::kOutOfMemory;
        return 0;
    }

    for (const auto& value : values) {
        list->push_back(value);
    }
    return list->size();
}

size_t Database::LLen(const std::string& key) {
    status_ = Status::kOk;
    if (!CheckListStatus(key)) {
        return 0;
    }
    ListValue* list = GetList(key);
    return list->size();
}

std::vector<std::string> Database::LPop(const std::string& key, long long count) {
    status_ = Status::kOk;
    std::vector<std::string> res;
    if (!CheckListStatus(key)) {
        return res;
    }
    ListValue* list = GetList(key, false);
    if (list->empty()) {
        return res;
    }

    auto it = data_.find(key);
    size_t before = ItemMemory(key, it->second);
    while (count > 0 && !list->empty()) {
        res.push_back(list->front());
        list->pop_front();
        --count;
    }
    size_t after = ItemMemory(key, it->second);
    if (after < before) {
        ReleaseMemory(before - after);
    }
    return res;
}

std::vector<std::string> Database::RPop(const std::string& key, long long count) {
    status_ = Status::kOk;
    std::vector<std::string> res;
    if (!CheckListStatus(key)) {
        return res;
    }
    ListValue* list = GetList(key, false);
    if (list->empty()) {
        return res;
    }

    auto it = data_.find(key);
    size_t before = ItemMemory(key, it->second);
    while (count > 0 && !list->empty()) {
        res.push_back(list->back());
        list->pop_back();
        --count;
    }
    size_t after = ItemMemory(key, it->second);
    if (after < before) {
        ReleaseMemory(before - after);
    }
    return res;
}

std::vector<std::string> Database::LRange(const std::string& key, long long start, long long stop) {
    status_ = Status::kOk;
    std::vector<std::string> res;
    if (!CheckListStatus(key)) {
        return res;
    }
    ListValue* list = GetList(key, false);
    if (list->empty()) {
        return res;
    }
    
    long long size = static_cast<long long>(list->size());
    start = NormalizeInd(start, size);
    stop = NormalizeInd(stop, size);

    if ((start > stop) || (stop < 0) || (start < 0) || (start >= size)) {
        return res;
    }

    for (long long i = start; i <= stop && i < size; ++i) {
        size_t index = static_cast<size_t>(i);
        res.push_back(list->at(index));
    }

    return res;
}

std::optional<std::string> Database::LIndex(const std::string& key, long long index) {
    status_ = Status::kOk;
    if (!CheckListStatus(key)) {
        return std::nullopt;
    }
    ListValue* list = GetList(key, false);
    if (list->empty()) {
        status_ = Status::kNotFound;
        return std::nullopt;
    }
    
    long long size = static_cast<long long>(list->size());
    index = NormalizeInd(index, size);

    if (index < 0 || index >= size) {
        status_ = Status::kIndexError;
        return std::nullopt;
    }

    size_t ind = static_cast<size_t>(index);
    return list->at(ind);
}

bool Database::LSet(const std::string& key, long long index, const std::string& value) {
    status_ = Status::kOk;
    if (!CheckListStatus(key)) {
        return false;
    }
    ListValue* list = GetList(key, false);
    if (list->empty()) {
        status_ = Status::kNotFound;
        return false;
    }

    long long size = static_cast<long long>(list->size());
    index = NormalizeInd(index, size);
    if (index < 0 || index >= size) {
        status_ = Status::kIndexError;
        return false;
    }

    size_t ind = static_cast<size_t>(index);
    const std::string& old_value = list->at(ind);
    size_t old_bytes = StringMemory(old_value);
    size_t new_bytes = StringMemory(value);

    if (new_bytes > old_bytes) {
        if (!CanGrow(new_bytes - old_bytes)) {
            status_ = Status::kOutOfMemory;
            return false;
        }
    }
    else {
        ReleaseMemory(old_bytes - new_bytes);
    }

    list->at(ind) = value;
    return true;
}

long long Database::LInsert(const std::string& key, InsertWhere where, const std::string& pivot, const std::string& value) {
    status_ = Status::kOk;
    if (!CheckListStatus(key)) {
        return 0;
    }
    ListValue* list = GetList(key, false);

    for (auto it = list->begin(); it != list->end(); ++it) {
        if (*it != pivot) {
            continue;
        }
        if (!CanGrow(StringMemory(value))) {
            status_ = Status::kOutOfMemory;
            return 0;
        }
        if (where == InsertWhere::kBefore) {
            list->insert(it, value);
        }
        else {
            ++it;
            list->insert(it, value);
        }
        return static_cast<long long>(list->size());
    }
    status_ = Status::kPivotNotFound;
    return -1;
}