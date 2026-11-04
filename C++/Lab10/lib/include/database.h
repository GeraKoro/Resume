#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <ostream>

namespace fs = std::filesystem;

using StringValue = std::string;
using ListValue = std::deque<std::string>;
using SetValue = std::unordered_set<std::string>;
using GeoValue = std::unordered_map<std::string, std::tuple<double, double>>;

using Value = std::variant<StringValue, ListValue, SetValue, GeoValue>;

struct Item {
    Value value;
    std::optional<std::chrono::steady_clock::time_point> expires_at;
};

enum class InsertWhere {
    kBefore,
    kAfter,
};


enum class GeoUnit {
    kM,
    kKm,
    kMi,
    kFt,
};


struct GeoSearchOptions {
    double lat = 0.0;
    double lon = 0.0;
    double radius = 0.0;
    GeoUnit unit = GeoUnit::kM;
    bool has_asc = false;
    bool asc = true;
    bool has_count = false;
    long long count = 0;
};

enum class Status {
    kOk,
    kNotFound,
    kWrongType,
    kOutOfMemory,
    kIndexError,
    kPivotNotFound,
};


class Database {
private:
    std::unordered_map<std::string, Item> data_;
    size_t max_memory_ = 0;
    size_t used_memory_ = 0;

    static constexpr size_t kMinMemory = sizeof(std::string);
    static constexpr size_t kMinMemoryGeoCoords = 32;
    bool oom_ = false;

    size_t StringMemory(const StringValue& string) const;
    size_t ListMemory(const ListValue& list) const;
    size_t SetMemory(const SetValue& set) const;
    size_t GeoMemory(const GeoValue& geo) const;
    size_t ItemMemory(const std::string& key, const Item& item) const;

    bool CanFit(size_t old_bytes, size_t new_bytes) const;
    void ReleaseMemory(size_t bytes);
    void RemoveMemory(const std::string& key, const Item& item);
    bool CanReplace(const std::string& key, Item item);
    bool CanGrow(size_t extra_bytes);

    bool IsExpired(const Item& item) const;
    void RemoveIfExpired(const std::string& key);
    Item* GetItem(const std::string& key);
    const Item* GetItem(const std::string& key) const;

    long long NormalizeInd(long long index, long long size);

    ListValue* GetList(const std::string& key, bool to_create = false);
    SetValue* GetSet(const std::string& key, bool to_create = false);
    GeoValue* GetGeo(const std::string& key, bool to_create = false);

    bool PatternMatches(const std::string& key, const std::string& pattern) const;

    bool CheckStringStatus(const std::string& key);
    bool CheckListStatus(const std::string& key);
    bool CheckSetStatus(const std::string& key);
    bool CheckGeoStatus(const std::string& key);
    Status status_ = Status::kOk;

public:

    bool GetOom() const { return oom_; }

    Status GetStatus() const { return status_; }

    void Set(const std::string& key, const std::string& value);
    std::string Get(const std::string& key);

    size_t Strlen(const std::string& key);
    
    size_t Append(const std::string& key, const std::string& value);
    
    long long Expire(const std::string& key, size_t seconds);
    long long TTL(const std::string& key);




    size_t LPush(const std::string& key, const std::vector<std::string>& values);
    size_t RPush(const std::string& key, const std::vector<std::string>& values);

    std::vector<std::string> LPop(const std::string& key, long long count = 1);
    std::vector<std::string> RPop(const std::string& key, long long count = 1);

    size_t LLen(const std::string& key);

    std::vector<std::string> LRange(const std::string& key, long long start, long long stop);
    std::optional<std::string> LIndex(const std::string& key, long long index);

    bool LSet(const std::string& key, long long index, const std::string& value);

    long long LInsert(const std::string& key, InsertWhere where, const std::string& pivot, const std::string& value);




    size_t SAdd(const std::string& key, const std::vector<std::string>& members);
    size_t SRem(const std::string& key, const std::vector<std::string>& members);

    bool SIsMember(const std::string& key, const std::string& member);
    std::vector<std::string> SMembers(const std::string& key);

    size_t SCard(const std::string& key);

    std::vector<std::string> SUnion(const std::vector<std::string>& keys);
    std::vector<std::string> SInter(const std::vector<std::string>& keys);
    std::vector<std::string> SDiff(const std::vector<std::string>& keys);
    
    size_t SMove(const std::string& source, const std::string& destination, const std::string& member);





    size_t GeoAdd(const std::string& key, const std::vector<std::tuple<double, double, std::string>>& points);
    
    std::vector<std::optional<std::tuple<double, double>>> GeoPos(const std::string& key, const std::vector<std::string>& members);
    
    std::optional<double> GeoDist(const std::string& key, const std::string& member1, const std::string& member2, GeoUnit unit);
    
    std::vector<std::string> GeoSearch(const std::string& key, const GeoSearchOptions& options);
    size_t GeoSearchStore(const std::string& dest, const std::string& source, const GeoSearchOptions& options);




    std::string Type(const std::string& key);

    size_t Del(const std::vector<std::string>& keys);
    size_t Exists(const std::vector<std::string>& keys);
    
    std::vector<std::string> Keys(const std::string& pattern);
    
    void FlushDb();
    size_t DbSize();
    
    void ConfigSet(std::size_t bytes);
    std::size_t ConfigGet() const;
    size_t MemoryUsage(const std::string& key);

    void Export(std::string path);
};  