#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

#include "cache_algorithms/chase.h"
#include "cache_algorithms/sattolo.h"
#include "test_helpers.h"

namespace {

std::unique_ptr<Node[]> shuffled(i64 n, u64 seed) {
    auto arr = std::make_unique<Node[]>(n);
    fill_array(arr.get(), n);
    std::mt19937_64 rng(seed);
    sattolo(arr.get(), n, rng);
    return arr;
}

} // namespace

class SattoloSizes : public ::testing::TestWithParam<i64> {};

TEST_P(SattoloSizes, InPlaceProducesOneCycleThroughEveryNode) {
    const i64 n = GetParam();
    auto arr = shuffled(n, 42);
    EXPECT_EQ(walk_cycle(&arr[0]), n);
}

TEST_P(SattoloSizes, CycleTableIsAPermutation) {
    const i64 n = GetParam();
    std::mt19937_64 rng(42);
    std::vector<u32> next = sattolo_cycle(n, rng);
    ASSERT_EQ(static_cast<i64>(next.size()), n);

    std::vector<u32> sorted = next;
    std::sort(sorted.begin(), sorted.end());
    std::vector<u32> expected(n);
    std::iota(expected.begin(), expected.end(), 0);
    EXPECT_EQ(sorted, expected);
}

TEST_P(SattoloSizes, CycleTableReturnsToZeroAfterNSteps) {
    const i64 n = GetParam();
    std::mt19937_64 rng(42);
    std::vector<u32> next = sattolo_cycle(n, rng);

    u32 cur = 0;
    i64 steps = 0;
    do {
        cur = next[cur];
        ++steps;
    } while (cur != 0 && steps <= n);
    EXPECT_EQ(steps, n);
}

INSTANTIATE_TEST_SUITE_P(Sizes, SattoloSizes, ::testing::Values(1, 2, 3, 64, 1000));

TEST(Sattolo, SingleNodeLinksToItself) {
    auto arr = shuffled(1, 7);
    EXPECT_EQ(arr[0].next, &arr[0]);
}

TEST(Sattolo, SameSeedGivesSameOrder) {
    const i64 n = 1000;
    auto a = shuffled(n, 99);
    auto b = shuffled(n, 99);
    for (i64 i = 0; i < n; ++i)
        EXPECT_EQ(a[i].next - a.get(), b[i].next - b.get());
}

TEST(Sattolo, DifferentSeedsGiveDifferentOrder) {
    const i64 n = 1000;
    auto a = shuffled(n, 1);
    auto b = shuffled(n, 2);
    i64 same = 0;
    for (i64 i = 0; i < n; ++i)
        same += (a[i].next - a.get()) == (b[i].next - b.get());
    EXPECT_LT(same, n);
}
