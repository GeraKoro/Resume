#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "lib/ttask.h"

#define ASSERT_EQ(a, b) REQUIRE((a) == (b))
#define ASSERT_TRUE(expr) REQUIRE(expr)
#define ASSERT_FALSE(expr) REQUIRE_FALSE(expr)
#define ASSERT_THROWS_AS(expr, exc) REQUIRE_THROWS_AS(expr, exc)

namespace {

int triple(int x) {
    return x * 3;
}

int sum2(int a, int b) {
    return a + b;
}

struct Holder {
    int value;
    int Mix(int x) const {
        return value * 2 + x;
    }
};

struct Foo {
    int Bar(int x) const {
        return x * 2;
    }
};

}

TEST_CASE("simple_add_and_executeAll") {
    TTaskScheduler s;
    auto t1 = s.add(triple, 5);
    auto t2 = s.add(sum2, 1, 2);
    s.executeAll();
    ASSERT_EQ(t1.getFutureResult().get(), 15);
    ASSERT_EQ(t2.getFutureResult().get(), 3);
}

TEST_CASE("executeAll_empty_scheduler") {
    TTaskScheduler s;
    s.executeAll();
}

TEST_CASE("future_not_ready_then_ready_after_execute") {
    TTaskScheduler s;
    auto t = s.add(triple, 5);
    auto f = t.getFutureResult<int>();
    ASSERT_FALSE(f.Ready());
    s.executeAll();
    ASSERT_TRUE(f.Ready());
    ASSERT_EQ(f.get(), 15);
}

TEST_CASE("future_get_before_ready_throws") {
    TTaskScheduler s;
    auto t = s.add(triple, 5);
    auto f = t.getFutureResult<int>();
    ASSERT_THROWS_AS((void)f.get(), ttask::FutureIsNotReady);
}

TEST_CASE("future_getProducer_non_null") {
    TTaskScheduler s;
    auto t = s.add(triple, 5);
    auto f = t.getFutureResult<int>();
    ASSERT_TRUE(f.GetProducer().get() != nullptr);
}

TEST_CASE("void_future_get_after_execute") {
    TTaskScheduler s;
    int side = 0;
    auto t = s.add([&side]() { side = 7; });
    auto fv = t.getFutureResult<void>();
    s.executeAll();
    fv.get();
    ASSERT_EQ(side, 7);
}

TEST_CASE("chained_via_TFuture_arg") {
    TTaskScheduler s;
    auto base = s.add(triple, 5);
    auto fut = base.getFutureResult();
    auto next = s.add([](int x) { return x + 1; }, fut);
    s.executeAll();
    ASSERT_EQ(next.getFutureResult().get(), 16);
}

TEST_CASE("chained_executeAll_reverse_order_still_works") {
    TTaskScheduler s;
    auto base = s.add(triple, 4);
    auto fut = base.getFutureResult();
    auto next = s.add([](int x) { return x * 10; }, fut);
    s.executeAll();
    ASSERT_EQ(next.getFutureResult().get(), 120);
}

TEST_CASE("two_dependencies_merge") {
    TTaskScheduler s;
    auto a = s.add([]() { return 10; });
    auto b = s.add([]() { return 3; });
    auto fa = a.getFutureResult<int>();
    auto fb = b.getFutureResult<int>();
    auto sum = s.add([](int x, int y) { return x + y; }, fa, fb);
    s.executeAll();
    ASSERT_EQ(sum.getFutureResult<int>().get(), 13);
}

TEST_CASE("apply_chains_through_scheduler") {
    TTaskScheduler s;
    auto base = s.add(triple, 7);
    auto next = base.apply([](int x) { return x - 1; });
    s.executeAll();
    ASSERT_EQ(next.getFutureResult().get(), 20);
}

TEST_CASE("apply_after_scheduler_destroyed_throws") {
    auto task = [] {
        TTaskScheduler s;
        return s.add(triple, 5);
    }();
    ASSERT_THROWS_AS((void)task.apply([](int x) { return x; }), ttask::EmptyScheduler);
}

TEST_CASE("pmf_member_function_pointer") {
    TTaskScheduler s;
    Holder h{5};
    auto t = s.add(&Holder::Mix, h, 3);
    s.executeAll();
    ASSERT_EQ(t.getFutureResult<int>().get(), 13);
}

TEST_CASE("pmf_via_pointer_to_object") {
    TTaskScheduler s;
    Foo f{};
    auto t = s.add(&Foo::Bar, &f, 3);
    s.executeAll();
    ASSERT_EQ(t.getFutureResult<int>().get(), 6);
}

TEST_CASE("scheduler_destroyed_before_lazy_sync_throws") {
    auto task = [] {
        TTaskScheduler sched;
        return sched.add([] { return 42; });
    }();
    ASSERT_THROWS_AS((void)task.getResultSync<int>(), ttask::EmptyScheduler);
}

TEST_CASE("const_ref_future_multi_get") {
    TTaskScheduler s;
    auto t = s.add(triple, 5);
    auto f1 = t.getFutureResult<const int&>();
    auto f2 = t.getFutureResult<const int&>();
    s.executeAll();
    ASSERT_EQ(f1.get(), 15);
    ASSERT_EQ(f2.get(), 15);
    ASSERT_EQ(f1.get(), 15);
}

TEST_CASE("value_future_double_get_throws") {
    TTaskScheduler s;
    auto t = s.add(triple, 5);
    s.executeAll();
    auto f = t.getFutureResult<int>();
    ASSERT_EQ(f.get(), 15);
    ASSERT_THROWS_AS((void)f.get(), ttask::FutureHasBeenAlreadyMoved);
}

TEST_CASE("future_get_then_sync_throws") {
    TTaskScheduler s;
    auto t = s.add(triple, 5);
    s.executeAll();
    auto f = t.getFutureResult<int>();
    ASSERT_EQ(f.get(), 15);
    ASSERT_THROWS_AS((void)t.getResultSync<int>(), ttask::FutureHasBeenAlreadyMoved);
}

TEST_CASE("lazy_sync_only_runs_branch") {
    TTaskScheduler s;
    int counter_a = 0, counter_b = 0;
    auto a = s.add([&counter_a]() {
        ++counter_a;
        return 1;
    });
    auto b = s.add([&counter_b]() {
        ++counter_b;
        return 2;
    });
    (void)b;
    ASSERT_EQ(a.getResultSync(), 1);
    ASSERT_EQ(counter_a, 1);
    ASSERT_EQ(counter_b, 0);
}

TEST_CASE("void_returning_task") {
    TTaskScheduler s;
    int side = 0;
    auto t = s.add([&side]() { side = 42; });
    s.executeAll();
    t.getResultSync<void>();
    ASSERT_EQ(side, 42);
}

TEST_CASE("cycle_detection_throws") {
    TTaskScheduler s;
    auto a = s.add([]() { return 1; });
    auto b = s.add([]() { return 2; });
    a.addAddiction(b);
    b.addAddiction(a);
    ASSERT_THROWS_AS(s.executeAll(), ttask::CycleInScheduler);
}

TEST_CASE("apply_chain_three_levels") {
    TTaskScheduler s;
    auto t1 = s.add(triple, 2);
    auto t2 = t1.apply([](int x) { return x + 1; });
    auto t3 = t2.apply([](int x) { return x * x; });
    s.executeAll();
    ASSERT_EQ(t3.getFutureResult<int>().get(), 49);
}

TEST_CASE("move_only_argument_forwarded_to_callable") {
    TTaskScheduler s;
    auto p = std::make_unique<int>(42);
    auto t = s.add([](std::unique_ptr<int> x) { return *x; }, std::move(p));
    s.executeAll();
    ASSERT_EQ(t.getFutureResult<int>().get(), 42);
}