#include "database.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

namespace {

std::vector<std::string> Sorted(std::vector<std::string> values) {
    std::sort(values.begin(), values.end());
    return values;
}

}  // namespace

TEST(StringTest, SetGet) {
    Database db;
    db.Set("k", "v");
    EXPECT_EQ(db.Get("k"), "v");
    EXPECT_EQ(db.GetStatus(), Status::kOk);
}

TEST(StringTest, GetMissing) {
    Database db;
    db.Get("missing");
    EXPECT_EQ(db.GetStatus(), Status::kNotFound);
}

TEST(StringTest, AppendAndStrlen) {
    Database db;
    db.Set("k", "ab");
    EXPECT_EQ(db.Append("k", "cd"), 4U);
    EXPECT_EQ(db.Get("k"), "abcd");
    EXPECT_EQ(db.Strlen("k"), 4U);
}

TEST(StringTest, AppendCreatesKey) {
    Database db;
    EXPECT_EQ(db.Append("new_key", "x"), 1U);
    EXPECT_EQ(db.Get("new_key"), "x");
}

TEST(StringTest, WrongType) {
    Database db;
    db.LPush("k", {"a"});
    db.Get("k");
    EXPECT_EQ(db.GetStatus(), Status::kWrongType);
}

TEST(StringTest, ExpireAndTtl) {
    Database db;
    db.Set("k", "v");
    EXPECT_EQ(db.Expire("k", 60), 1);
    EXPECT_GE(db.TTL("k"), 59);
    EXPECT_EQ(db.TTL("no_key"), -2);
    db.Set("k2", "v");
    EXPECT_EQ(db.TTL("k2"), -1);
}

TEST(ListTest, PushPopAndLen) {
    Database db;
    EXPECT_EQ(db.LPush("lst", {"a", "b"}), 2U);
    EXPECT_EQ(db.RPush("lst", {"c"}), 3U);
    EXPECT_EQ(db.LLen("lst"), 3U);
    auto left = db.LPop("lst", 2);
    ASSERT_EQ(left.size(), 2U);
    EXPECT_EQ(left[0], "b");
    EXPECT_EQ(left[1], "a");
    auto right = db.RPop("lst", 1);
    ASSERT_EQ(right.size(), 1U);
    EXPECT_EQ(right[0], "c");
}

TEST(ListTest, RangeAndIndex) {
    Database db;
    db.RPush("lst", {"a", "b", "c", "d"});
    auto range = db.LRange("lst", 1, 2);
    ASSERT_EQ(range.size(), 2U);
    EXPECT_EQ(range[0], "b");
    EXPECT_EQ(range[1], "c");
    auto item = db.LIndex("lst", -1);
    ASSERT_TRUE(item);
    EXPECT_EQ(*item, "d");
}

TEST(ListTest, Insert) {
    Database db;
    db.RPush("lst", {"a", "b", "c"});
    EXPECT_EQ(db.LInsert("lst", InsertWhere::kBefore, "b", "x"), 4);
    auto range = db.LRange("lst", 0, -1);
    ASSERT_EQ(range.size(), 4U);
    EXPECT_EQ(range[1], "x");
    EXPECT_EQ(db.LInsert("lst", InsertWhere::kAfter, "z", "y"), -1);
    EXPECT_EQ(db.GetStatus(), Status::kPivotNotFound);
}

TEST(SetTest, AddRemoveMember) {
    Database db;
    EXPECT_EQ(db.SAdd("s", {"a", "b", "a"}), 2U);
    EXPECT_TRUE(db.SIsMember("s", "a"));
    EXPECT_EQ(db.SCard("s"), 2U);
    EXPECT_EQ(db.SRem("s", {"a"}), 1U);
    EXPECT_FALSE(db.SIsMember("s", "a"));
}

TEST(SetTest, UnionInterDiff) {
    Database db;
    db.SAdd("a", {"1", "2"});
    db.SAdd("b", {"2", "3"});
    EXPECT_EQ(Sorted(db.SUnion({"a", "b"})), Sorted({"1", "2", "3"}));
    EXPECT_EQ(Sorted(db.SInter({"a", "b"})), Sorted({"2"}));
    EXPECT_EQ(Sorted(db.SDiff({"a", "b"})), Sorted({"1"}));
}

TEST(SetTest, Move) {
    Database db;
    db.SAdd("src", {"m"});
    EXPECT_EQ(db.SMove("src", "dst", "m"), 1U);
    EXPECT_FALSE(db.SIsMember("src", "m"));
    EXPECT_TRUE(db.SIsMember("dst", "m"));
}

TEST(GeoTest, AddPosDist) {
    Database db;
    std::vector<std::tuple<double, double, std::string>> points = {{52.5, 13.4, "berlin"}, {48.8, 2.3, "paris"}};
    EXPECT_EQ(db.GeoAdd("city", points), 2U);
    EXPECT_EQ(db.Type("city"), "zset");
    auto pos = db.GeoPos("city", {"berlin", "missing"});
    ASSERT_EQ(pos.size(), 2U);
    ASSERT_TRUE(pos[0]);
    EXPECT_DOUBLE_EQ(std::get<0>(*pos[0]), 52.5);
    EXPECT_DOUBLE_EQ(std::get<1>(*pos[0]), 13.4);
    EXPECT_FALSE(pos[1]);
    auto dist = db.GeoDist("city", "berlin", "berlin", GeoUnit::kM);
    ASSERT_TRUE(dist);
    EXPECT_NEAR(*dist, 0.0, 1e-6);
}

TEST(GeoTest, SearchAndStore) {
    Database db;
    db.GeoAdd("city", {{52.5, 13.4, "berlin"}, {48.8, 2.3, "paris"}});
    GeoSearchOptions options;
    options.lat = 52.5;
    options.lon = 13.4;
    options.radius = 1000.0;
    options.unit = GeoUnit::kM;
    auto found = db.GeoSearch("city", options);
    ASSERT_EQ(found.size(), 1U);
    EXPECT_EQ(found[0], "berlin");
    options.has_count = true;
    options.count = 1;
    options.has_asc = true;
    options.asc = true;
    EXPECT_EQ(db.GeoSearchStore("out", "city", options), 1U);
    EXPECT_EQ(db.Type("out"), "zset");
    EXPECT_TRUE(db.GeoPos("out", {"berlin"})[0]);
}

TEST(GeneralTest, TypeDelExists) {
    Database db;
    db.Set("a", "1");
    db.LPush("b", {"x"});
    EXPECT_EQ(db.Type("a"), "string");
    EXPECT_EQ(db.Type("b"), "list");
    EXPECT_EQ(db.Type("none"), "none");
    EXPECT_EQ(db.Exists({"a", "b", "z"}), 2U);
    EXPECT_EQ(db.Del({"a"}), 1U);
    EXPECT_EQ(db.Exists({"a"}), 0U);
}

TEST(GeneralTest, KeysPattern) {
    Database db;
    db.Set("foo", "1");
    db.Set("bar", "2");
    db.Set("food", "3");
    auto keys = Sorted(db.Keys("foo*"));
    EXPECT_EQ(keys, Sorted({"foo", "food"}));
}

TEST(GeneralTest, FlushAndDbSize) {
    Database db;
    db.Set("a", "1");
    db.Set("b", "2");
    EXPECT_EQ(db.DbSize(), 2U);
    db.FlushDb();
    EXPECT_EQ(db.DbSize(), 0U);
}

TEST(MemoryTest, ConfigAndUsage) {
    Database db;
    db.ConfigSet(1024);
    EXPECT_EQ(db.ConfigGet(), 1024U);
    db.Set("k", "data");
    EXPECT_GT(db.MemoryUsage("k"), 0U);
}

TEST(MemoryTest, OomOnSet) {
    Database db;
    db.ConfigSet(32);
    db.Set("k", std::string(200, 'x'));
    EXPECT_EQ(db.GetStatus(), Status::kOutOfMemory);
    EXPECT_TRUE(db.GetOom());
}

TEST(MemoryTest, OomOnLPush) {
    Database db;
    db.ConfigSet(64);
    db.LPush("lst", {std::string(100, 'a')});
    EXPECT_EQ(db.GetStatus(), Status::kOutOfMemory);
}

TEST(MemoryTest, UsageMissingKey) {
    Database db;
    EXPECT_EQ(db.MemoryUsage("missing"), 0U);
}

TEST(ListTest, LSet) {
    Database db;
    db.RPush("lst", {"a", "b"});
    ASSERT_TRUE(db.LSet("lst", 1, "x"));
    auto item = db.LIndex("lst", 1);
    ASSERT_TRUE(item);
    EXPECT_EQ(*item, "x");
}

TEST(ListTest, LIndexOutOfRange) {
    Database db;
    db.RPush("lst", {"a"});
    EXPECT_FALSE(db.LIndex("lst", 5));
    EXPECT_EQ(db.GetStatus(), Status::kIndexError);
}

TEST(ListTest, PopEmptyList) {
    Database db;
    db.RPush("lst", {"a"});
    db.LPop("lst", 1);
    auto res = db.LPop("lst", 1);
    EXPECT_TRUE(res.empty());
}

TEST(SetTest, SMembers) {
    Database db;
    db.SAdd("s", {"c", "a", "b"});
    EXPECT_EQ(Sorted(db.SMembers("s")), Sorted({"a", "b", "c"}));
}

TEST(GeoTest, GeoAddUpdate) {
    Database db;
    EXPECT_EQ(db.GeoAdd("city", {{52.5, 13.4, "berlin"}}), 1U);
    EXPECT_EQ(db.GeoAdd("city", {{50.0, 10.0, "berlin"}}), 0U);
    auto pos = db.GeoPos("city", {"berlin"});
    ASSERT_TRUE(pos[0]);
    EXPECT_DOUBLE_EQ(std::get<0>(*pos[0]), 50.0);
    EXPECT_DOUBLE_EQ(std::get<1>(*pos[0]), 10.0);
}

TEST(GeoTest, GeoDistKm) {
    Database db;
    db.GeoAdd("city", {{52.5, 13.4, "berlin"}, {48.8, 2.3, "paris"}});
    auto dist = db.GeoDist("city", "berlin", "paris", GeoUnit::kKm);
    ASSERT_TRUE(dist);
    EXPECT_NEAR(*dist, 878.0, 20.0);
}

TEST(GeoTest, GeoDistMissingMember) {
    Database db;
    db.GeoAdd("city", {{52.5, 13.4, "berlin"}});
    EXPECT_FALSE(db.GeoDist("city", "berlin", "paris", GeoUnit::kKm));
}

TEST(GeoTest, GeoSearchDesc) {
    Database db;
    db.GeoAdd("city", {{52.5, 13.4, "near"}, {48.8, 2.3, "far"}});
    GeoSearchOptions options;
    options.lat = 52.5;
    options.lon = 13.4;
    options.radius = 2000000.0;
    options.unit = GeoUnit::kM;
    options.has_asc = true;
    options.asc = false;
    auto found = db.GeoSearch("city", options);
    ASSERT_EQ(found.size(), 2U);
    EXPECT_EQ(found[0], "far");
}

TEST(GeneralTest, KeysQuestionMark) {
    Database db;
    db.Set("foo", "1");
    db.Set("bar", "2");
    db.Set("baz", "3");
    EXPECT_EQ(Sorted(db.Keys("?oo")), Sorted({"foo"}));
}

TEST(GeneralTest, KeysAll) {
    Database db;
    db.Set("a", "1");
    db.Set("b", "2");
    EXPECT_EQ(Sorted(db.Keys("*")), Sorted({"a", "b"}));
}

TEST(GeneralTest, ExpireOnList) {
    Database db;
    db.LPush("lst", {"x"});
    EXPECT_EQ(db.Expire("lst", 30), 1);
    EXPECT_GE(db.TTL("lst"), 29);
}