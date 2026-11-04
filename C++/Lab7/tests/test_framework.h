#pragma once

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>

void RegisterTest(std::string name, std::function<void()> fn);

#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b)       TEST_CONCAT_INNER(a, b)

#define TEST_CASE(name)                                                                                    \
    static void TEST_CONCAT(test_fn_, __LINE__)();                                                         \
    static struct TEST_CONCAT(test_reg_, __LINE__) {                                                       \
        TEST_CONCAT(test_reg_, __LINE__)() { RegisterTest(name, TEST_CONCAT(test_fn_, __LINE__)); }        \
    } TEST_CONCAT(test_reg_inst_, __LINE__);                                                               \
    static void TEST_CONCAT(test_fn_, __LINE__)()

inline void AssertFail(const char* expr, const std::string& detail = {}) {
    std::ostringstream o;
    o << "ASSERT failed: " << expr;
    if (!detail.empty()) {
        o << " (" << detail << ')';
    }
    throw std::runtime_error(o.str());
}

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            AssertFail(#cond); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) \
    do { \
        const auto& assert_eq_a = (a); \
        const auto& assert_eq_b = (b); \
        if (!(assert_eq_a == assert_eq_b)) { \
            std::ostringstream o; \
            o << #a << " != " << #b; \
            AssertFail("ASSERT_EQ", o.str()); \
        } \
    } while (0)
