#include "parser.h"

#include "../lib/include/database.h"

#include <cctype>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

static bool IgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

static std::vector<std::string> LineToWords(const std::string& line) {
    std::vector<std::string> words;
    std::string word;
    for (char c : line) {
        if (c == ' ' || c == '\t') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

static void PrintNumber(long long x) {
    std::cout << x << '\n';
}

static void PrintStr(const std::string& s) {
    std::cout << s << '\n';
}

static void PrintNull() {
    std::cout << "(null)\n";
}

static void PrintArray(const std::vector<std::string>& arr) {
    std::cout << '*' << arr.size() << '\n';
    for (const auto& x : arr) {
        std::cout << x << '\n';
    }
}

static void PrintErrUnknown() {
    std::cerr << "(error) Unknown command or wrong number of arguments\n";
}

static void PrintErrWrongType() {
    std::cerr << "(error) WRONGTYPE This key holding another value type\n";
}

static void PrintErrOOM() {
    std::cerr << "(error) OOM command not allowed when used memory > 'maxmemory'\n";
}

static bool CheckDbError(Database& db) {
    if (db.GetStatus() == Status::kWrongType) {
        PrintErrWrongType();
        return true;
    }
    if (db.GetStatus() == Status::kOutOfMemory) {
        PrintErrOOM();
        return true;
    }
    return false;
}

static std::string GetCommand(const std::vector<std::string>& words, size_t& arg_i) {
    std::string command = words[0];
    arg_i = 1;
    if (words.size() >= 2 && IgnoreCase(words[0], "config")) {
        command = words[0] + " " + words[1];
        arg_i = 2;
    }
    if (words.size() >= 2 && IgnoreCase(words[0], "memory")) {
        command = words[0] + " " + words[1];
        arg_i = 2;
    }
    return command;
}

static long long ParseLong(const std::string& s) {
    return std::stoll(s);
}

static double ParseDouble(const std::string& s) {
    return std::stod(s);
}

static GeoUnit ParseGeoUnit(const std::string& s) {
    if (IgnoreCase(s, "km")) {
        return GeoUnit::kKm;
    }
    if (IgnoreCase(s, "mi")) {
        return GeoUnit::kMi;
    }
    if (IgnoreCase(s, "ft")) {
        return GeoUnit::kFt;
    }
    return GeoUnit::kM;
}

static bool ParseGeoSearchOptions(const std::vector<std::string>& words, size_t start, GeoSearchOptions& options) {
    if (words.size() < start + 6) {
        return false;
    }
    if (!IgnoreCase(words[start], "fromlonlat") || !IgnoreCase(words[start + 3], "byradius")) {
        return false;
    }
    options.lon = ParseDouble(words[start + 1]);
    options.lat = ParseDouble(words[start + 2]);
    options.radius = ParseDouble(words[start + 4]);
    options.unit = ParseGeoUnit(words[start + 5]);
    size_t i = start + 6;
    while (i < words.size()) {
        if (IgnoreCase(words[i], "asc")) {
            options.has_asc = true;
            options.asc = true;
            ++i;
        }
        else if (IgnoreCase(words[i], "desc")) {
            options.has_asc = true;
            options.asc = false;
            ++i;
        }
        else if (IgnoreCase(words[i], "count") && i + 1 < words.size()) {
            options.has_count = true;
            options.count = ParseLong(words[i + 1]);
            i += 2;
        }
        else {
            return false;
        }
    }
    return true;
}

void ReadLines(Database& database) {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::vector<std::string> parsed_line = LineToWords(line);
        if (parsed_line.empty()) {
            continue;
        }
        if (IgnoreCase(parsed_line[0], "exit")) {
            break;
        }
        ParseLine(database, parsed_line);
    }
}

void ParseLine(Database& database, const std::vector<std::string>& words) {
    try {
    size_t arg_i = 0;
    std::string command = GetCommand(words, arg_i);
    const size_t argc = words.size() - arg_i;

    auto arg = [&](size_t i) -> const std::string& {
        return words[arg_i + i];
    };

    if (IgnoreCase(command, "set") && argc == 2) {
        database.Set(arg(0), arg(1));
        if (CheckDbError(database)) {
            return;
        }
        PrintStr("OK");
        return;
    }

    if (IgnoreCase(command, "get") && argc == 1) {
        std::string value = database.Get(arg(0));
        if (database.GetStatus() == Status::kNotFound) {
            PrintNull();
            return;
        }
        if (CheckDbError(database)) {
            return;
        }
        PrintStr(value);
        return;
    }

    if (IgnoreCase(command, "strlen") && argc == 1) {
        size_t len = database.Strlen(arg(0));
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(static_cast<long long>(len));
        return;
    }

    if (IgnoreCase(command, "append") && argc == 2) {
        size_t len = database.Append(arg(0), arg(1));
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(static_cast<long long>(len));
        return;
    }

    if (IgnoreCase(command, "expire") && argc == 2) {
        long long r = database.Expire(arg(0), static_cast<size_t>(ParseLong(arg(1))));
        PrintNumber(r);
        return;
    }

    if (IgnoreCase(command, "ttl") && argc == 1) {
        long long time = database.TTL(arg(0));
        PrintNumber(time);
        return;
    }

    if (IgnoreCase(command, "type") && argc == 1) {
        PrintStr(database.Type(arg(0)));
        return;
    }

    if (IgnoreCase(command, "del") && argc >= 1) {
        std::vector<std::string> keys;
        for (size_t i = 0; i < argc; ++i) {
            keys.push_back(arg(i));
        }
        PrintNumber(static_cast<long long>(database.Del(keys)));
        return;
    }

    if (IgnoreCase(command, "exists") && argc >= 1) {
        std::vector<std::string> keys;
        for (size_t i = 0; i < argc; ++i) {
            keys.push_back(arg(i));
        }
        PrintNumber(static_cast<long long>(database.Exists(keys)));
        return;
    }

    if (IgnoreCase(command, "keys") && argc == 1) {
        PrintArray(database.Keys(arg(0)));
        return;
    }

    if (IgnoreCase(command, "flushdb") && argc == 0) {
        database.FlushDb();
        PrintStr("OK");
        return;
    }

    if (IgnoreCase(command, "config set") && argc == 2 && IgnoreCase(arg(0), "maxmemory")) {
        database.ConfigSet(static_cast<size_t>(ParseLong(arg(1))));
        PrintStr("OK");
        return;
    }


    if (IgnoreCase(command, "config get") && argc == 1 && IgnoreCase(arg(0), "maxmemory")) {
        PrintNumber(static_cast<long long>(database.ConfigGet()));
        return;
    }

    if (IgnoreCase(command, "dbsize") && argc == 0) {
        PrintNumber(static_cast<long long>(database.DbSize()));
        return;
    }

    if (IgnoreCase(command, "memory usage") && argc == 1) {
        PrintNumber(static_cast<long long>(database.MemoryUsage(arg(0))));
        return;
    }

    if ((IgnoreCase(command, "lpush") || IgnoreCase(command, "rpush")) && argc >= 2) {
        std::vector<std::string> vals;
        for (size_t i = 1; i < argc; ++i) {
            vals.push_back(arg(i));
        }
        size_t n = IgnoreCase(command, "lpush") ? database.LPush(arg(0), vals) : database.RPush(arg(0), vals);
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(static_cast<long long>(n));
        return;
    }

    if ((IgnoreCase(command, "lpop") || IgnoreCase(command, "rpop")) && argc >= 1) {
        long long count = 1;
        if (argc == 2) {
            count = ParseLong(arg(1));
        }
        std::vector<std::string> res = IgnoreCase(command, "lpop") ? database.LPop(arg(0), count) : database.RPop(arg(0), count);
        if (CheckDbError(database)) {
            return;
        }
        PrintArray(res);
        return;
    }

    if (IgnoreCase(command, "llen") && argc == 1) {
        size_t num = database.LLen(arg(0));
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(static_cast<long long>(num));
        return;
    }

    if (IgnoreCase(command, "lrange") && argc == 3) {
        auto res = database.LRange(arg(0), ParseLong(arg(1)), ParseLong(arg(2)));
        if (CheckDbError(database)) {
            return;
        }
        PrintArray(res);
        return;
    }

    if (IgnoreCase(command, "lindex") && argc == 2) {
        auto v = database.LIndex(arg(0), ParseLong(arg(1)));
        if (database.GetStatus() == Status::kNotFound
            || database.GetStatus() == Status::kIndexError) {
            PrintNull();
            return;
        }
        if (CheckDbError(database)) {
            return;
        }
        PrintStr(*v);
        return;
    }

    if (IgnoreCase(command, "lset") && argc == 3) {
        bool ok = database.LSet(arg(0), ParseLong(arg(1)), arg(2));
        if (CheckDbError(database)) {
            return;
        }
        if (!ok) {
            return;
        }
        PrintStr("OK");
        return;
    }

    if (IgnoreCase(command, "linsert") && argc == 4) {
        InsertWhere where = IgnoreCase(arg(1), "before") ? InsertWhere::kBefore : InsertWhere::kAfter;
        long long r = database.LInsert(arg(0), where, arg(2), arg(3));
        if (database.GetStatus() == Status::kPivotNotFound) {
            PrintNumber(-1);
            return;
        }
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(r);
        return;
    }

    if ((IgnoreCase(command, "sadd") || IgnoreCase(command, "srem")) && argc >= 2) {
        std::vector<std::string> members;
        for (size_t i = 1; i < argc; ++i) {
            members.push_back(arg(i));
        }
        size_t n = IgnoreCase(command, "sadd") ? database.SAdd(arg(0), members) : database.SRem(arg(0), members);
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(static_cast<long long>(n));
        return;
    }

    if (IgnoreCase(command, "sismember") && argc == 2) {
        bool r = database.SIsMember(arg(0), arg(1));
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(r ? 1 : 0);
        return;
    }

    if (IgnoreCase(command, "smembers") && argc == 1) {
        auto r = database.SMembers(arg(0));
        if (CheckDbError(database)) {
            return;
        }
        PrintArray(r);
        return;
    }

    if (IgnoreCase(command, "scard") && argc == 1) {
        size_t n = database.SCard(arg(0));
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(static_cast<long long>(n));
        return;
    }

    if ((IgnoreCase(command, "sunion") || IgnoreCase(command, "sinter") || IgnoreCase(command, "sdiff")) && argc >= 1) {
        std::vector<std::string> keys;
        for (size_t i = 0; i < argc; ++i) {
            keys.push_back(arg(i));
        }
        std::vector<std::string> r;
        if (IgnoreCase(command, "sunion")) {
            r = database.SUnion(keys);
        }
        else if (IgnoreCase(command, "sinter")) {
            r = database.SInter(keys);
        }
        else {
            r = database.SDiff(keys);
        }
        if (CheckDbError(database)) {
            return;
        }
        PrintArray(r);
        return;
    }

    if (IgnoreCase(command, "smove") && argc == 3) {
        size_t n = database.SMove(arg(0), arg(1), arg(2));
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(static_cast<long long>(n));
        return;
    }

    if (IgnoreCase(command, "geoadd") && argc >= 4 && (argc - 1) % 3 == 0) {
        std::vector<std::tuple<double, double, std::string>> points;
        for (size_t i = 1; i < argc; i += 3) {
            double lon = ParseDouble(arg(i));
            double lat = ParseDouble(arg(i + 1));
            points.emplace_back(lat, lon, arg(i + 2));
        }
        size_t n = database.GeoAdd(arg(0), points);
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(static_cast<long long>(n));
        return;
    }

    if (IgnoreCase(command, "geopos") && argc >= 2) {
        std::vector<std::string> members;
        for (size_t i = 1; i < argc; ++i) {
            members.push_back(arg(i));
        }
        auto res = database.GeoPos(arg(0), members);
        if (CheckDbError(database)) {
            return;
        }
        std::cout << '*' << res.size() << '\n';
        for (const auto& p : res) {
            if (!p) {
                PrintNull();
            }
            else {
                auto [lat, lon] = *p;
                std::cout << lon << ' ' << lat << '\n';
            }
        }
        return;
    }

    if (IgnoreCase(command, "geodist") && argc >= 3) {
        GeoUnit unit = GeoUnit::kM;
        std::string m1 = arg(1);
        std::string m2 = arg(2);
        if (argc == 4) {
            unit = ParseGeoUnit(arg(3));
        }
        auto d = database.GeoDist(arg(0), m1, m2, unit);
        if (CheckDbError(database)) {
            return;
        }
        if (!d) {
            PrintNull();
            return;
        }
        std::cout << *d << '\n';
        return;
    }

    if (IgnoreCase(command, "geosearch") && argc >= 7) {
        GeoSearchOptions options;
        if (!ParseGeoSearchOptions(words, arg_i + 1, options)) {
            PrintErrUnknown();
            return;
        }
        auto res = database.GeoSearch(arg(0), options);
        if (CheckDbError(database)) {
            return;
        }
        PrintArray(res);
        return;
    }

    if (IgnoreCase(command, "geosearchstore") && argc >= 8) {
        GeoSearchOptions options;
        if (!ParseGeoSearchOptions(words, arg_i + 2, options)) {
            PrintErrUnknown();
            return;
        }
        size_t n = database.GeoSearchStore(arg(0), arg(1), options);
        if (CheckDbError(database)) {
            return;
        }
        PrintNumber(static_cast<long long>(n));
        return;
    }

    PrintErrUnknown();
    }
    catch (...) {
        PrintErrUnknown();
    }
}

static bool IsSuf(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) {
        return false;
    }
    for (size_t i = 0; i < suffix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[s.size() - suffix.size() + i])) != std::tolower(static_cast<unsigned char>(suffix[i]))) {
            return false;
        }
    }
    return true;
}

static size_t ParseMaxMemory(const std::string& s) {
    size_t multiplier = 1;
    std::string number = s;
    if (IsSuf(s, "gb")) {
        multiplier = 1024ULL * 1024 * 1024;
        number = s.substr(0, s.size() - 2);
    }
    else if (IsSuf(s, "mb")) {
        multiplier = 1024ULL * 1024;
        number = s.substr(0, s.size() - 2);
    }
    else if (IsSuf(s, "kb")) {
        multiplier = 1024ULL;
        number = s.substr(0, s.size() - 2);
    }
    else if (IsSuf(s, "b")) {
        number = s.substr(0, s.size() - 1);
    }
    return static_cast<size_t>(std::stoull(number)) * multiplier;
}

void ApplyCommandLine(Database& database, int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--maxmemory" && i + 1 < argc) {
            try {
                database.ConfigSet(ParseMaxMemory(argv[i + 1]));
            }
            catch (...) {
                PrintErrUnknown();
            }
            ++i;
        }
    }
}