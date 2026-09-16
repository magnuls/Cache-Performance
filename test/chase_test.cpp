#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

#include "cache_algorithms/chase.h"
#include "cache_algorithms/sattolo.h"
#include "test_helpers.h"

namespace {

constexpr i64 kCount = 256;

std::unique_ptr<Node[]> shuffled(i64 n, u64 seed = 42) {
    auto arr = std::make_unique<Node[]>(n);
    fill_array(arr.get(), n);
    std::mt19937_64 rng(seed);
    sattolo(arr.get(), n, rng);
    return arr;
}

bool all_writeto_equal(const Node* arr, i64 n, u64 v) {
    for (i64 i = 0; i < n; ++i)
        if (arr[i].writeto != v) return false;
    return true;
}

} // namespace

TEST(FillArray, LinksEveryNodeToItself) {
    auto arr = std::make_unique<Node[]>(kCount);
    fill_array(arr.get(), kCount);
    for (i64 i = 0; i < kCount; ++i) {
        EXPECT_EQ(arr[i].next, &arr[i]);
        EXPECT_EQ(arr[i].writeto, 1u);
    }
}

TEST(FillArray, ResetsPreviouslyShuffledNodes) {
    auto arr = shuffled(kCount);
    arr[3].writeto = 77;
    fill_array(arr.get(), kCount);
    EXPECT_EQ(arr[3].next, &arr[3]);
    EXPECT_EQ(arr[3].writeto, 1u);
}

TEST(TotalAccesses, ClampsToTenAndFiftyMillion) {
    EXPECT_EQ(total_accesses(1), 10000000);
    EXPECT_EQ(total_accesses(1000000), 10000000);
    EXPECT_EQ(total_accesses(2000000), 20000000);
    EXPECT_EQ(total_accesses(5000000), 50000000);
    EXPECT_EQ(total_accesses(10000000), 50000000);
}

TEST(TimedAccess, ReadReturnsFiniteNonNegativeAndWritesNothing) {
    auto arr = shuffled(kCount);
    f64 ns = timed_access(arr.get(), kCount * 4, Sweep::Size);
    EXPECT_TRUE(std::isfinite(ns));
    EXPECT_GE(ns, 0.0);
    EXPECT_TRUE(all_writeto_equal(arr.get(), kCount, 1u));
}

TEST(TimedAccess, WriteVisitsEveryNodeOnce) {
    auto arr = shuffled(kCount);
    timed_access(arr.get(), kCount, Sweep::Write);

    std::vector<u64> seen;
    for (i64 i = 0; i < kCount; ++i) seen.push_back(arr[i].writeto);
    std::sort(seen.begin(), seen.end());
    for (i64 i = 0; i < kCount; ++i) EXPECT_EQ(seen[i], static_cast<u64>(i));
}

TEST(TimedAccess, WriteWrapsAroundTheCycle) {
    auto arr = shuffled(kCount);
    timed_access(arr.get(), kCount * 2, Sweep::Write);
    for (i64 i = 0; i < kCount; ++i) {
        EXPECT_GE(arr[i].writeto, static_cast<u64>(kCount));
        EXPECT_LT(arr[i].writeto, static_cast<u64>(kCount * 2));
    }
}

TEST(TimedAccess, UnhandledSweepsReturnSentinelAndTouchNothing) {
    auto arr = shuffled(kCount);
    EXPECT_EQ(timed_access(arr.get(), kCount, Sweep::LineSize), F64_MIN);
    EXPECT_EQ(timed_access(arr.get(), kCount, Sweep::Thread), F64_MIN);
    EXPECT_TRUE(all_writeto_equal(arr.get(), kCount, 1u));
}

TEST(WarmLoop, LeavesArrayUntouched) {
    auto identity = std::make_unique<Node[]>(kCount);
    fill_array(identity.get(), kCount);
    warm_loop(identity.get(), kCount);
    for (i64 i = 0; i < kCount; ++i) EXPECT_EQ(identity[i].next, &identity[i]);

    auto arr = shuffled(kCount);
    std::vector<Node*> before;
    for (i64 i = 0; i < kCount; ++i) before.push_back(arr[i].next);
    warm_loop(arr.get(), kCount);
    for (i64 i = 0; i < kCount; ++i) EXPECT_EQ(arr[i].next, before[i]);
    EXPECT_TRUE(all_writeto_equal(arr.get(), kCount, 1u));
}
