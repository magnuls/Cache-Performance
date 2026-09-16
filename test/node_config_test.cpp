#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include "cache_algorithms/config.h"
#include "cache_algorithms/node.h"
#include "types.h"

namespace {

constexpr bool is_pow2(i64 v) { return v > 0 && (v & (v - 1)) == 0; }

} // namespace

TEST(Node, FillsExactlyOneCacheLine) {
    EXPECT_EQ(sizeof(Node), static_cast<size_t>(kcache_line_size));
    EXPECT_EQ(alignof(Node), static_cast<size_t>(kcache_line_size));
    EXPECT_TRUE(is_pow2(kcache_line_size));
}

TEST(Node, DefaultWritetoIsOne) {
    Node n;
    EXPECT_EQ(n.writeto, 1u);
}

TEST(Node, HeapArrayElementsAreLineAligned) {
    auto arr = std::make_unique<Node[]>(8);
    for (i64 i = 0; i < 8; ++i)
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&arr[i]) % kcache_line_size, 0u);
}

TEST(Config, SweepBoundsArePowersOfTwo) {
    EXPECT_TRUE(is_pow2(STARTING_SET_READ));
    EXPECT_TRUE(is_pow2(ENDING_SET_READ));
    EXPECT_TRUE(is_pow2(STARTING_SET_WRITE));
    EXPECT_TRUE(is_pow2(ENDING_SET_WRITE));
    EXPECT_TRUE(is_pow2(START_STRIDE_LENGTH));
    EXPECT_TRUE(is_pow2(END_STRIDE_LENGTH));
}

TEST(Config, SweepsRunUpward) {
    EXPECT_LT(STARTING_SET_READ, ENDING_SET_READ);
    EXPECT_LT(STARTING_SET_WRITE, ENDING_SET_WRITE);
    EXPECT_LT(START_STRIDE_LENGTH, END_STRIDE_LENGTH);
}

TEST(Config, SmallestWorkingSetHoldsAtLeastOneNode) {
    EXPECT_GE(STARTING_SET_READ, static_cast<i64>(sizeof(Node)));
}

// cache_write_latency subtracts read_measurements by index, so both sweeps
// must step through identical sizes.
TEST(Config, WriteSweepMatchesReadSweep) {
    EXPECT_EQ(STARTING_SET_WRITE, STARTING_SET_READ);
    EXPECT_EQ(ENDING_SET_WRITE, ENDING_SET_READ);
}

TEST(Config, StrideStartsAtOrAboveOneWord) {
    EXPECT_GE(START_STRIDE_LENGTH, static_cast<i64>(sizeof(u32)));
}

TEST(Config, AtLeastOneTrial) {
    EXPECT_GT(TRIALS, 0);
}
