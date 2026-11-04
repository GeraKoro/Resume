#include "test_framework.h"

#include "cache.h"

#include <chrono>
#include <thread>

TEST_CASE("cache_put_get_basic") {
    ApiCache cache(10, std::chrono::minutes(10));
    cache.Put("k1", "v1");
    const auto x = cache.Get("k1");
    ASSERT_TRUE(x.has_value());
    ASSERT_EQ(*x, std::string("v1"));
}

TEST_CASE("cache_overflow_lru_eviction") {
    ApiCache cache(2, std::chrono::minutes(10));
    cache.Put("a", "1");
    cache.Put("b", "2");

    ASSERT_TRUE(cache.Get("a").has_value());
    cache.Put("c", "3");

    ASSERT_TRUE(cache.Get("a").has_value());
    ASSERT_FALSE(cache.Get("b").has_value());
    ASSERT_TRUE(cache.Get("c").has_value());
}

TEST_CASE("cache_expiration") {
    ApiCache cache(10, std::chrono::milliseconds(1));
    cache.Put("k", "v");
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    ASSERT_FALSE(cache.Get("k").has_value());
}
