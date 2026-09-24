#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "cache_algorithms/threads.h"

TEST(SpinBarrier, AllThreadsPassEachGeneration) {
    constexpr i32 n = 8;
    constexpr i32 generations = 3;
    SpinBarrier barrier(n);
    std::atomic<i32> counter{0};
    std::atomic<i32> violations{0};

    std::vector<std::thread> threads;
    for (i32 t = 0; t < n; ++t) {
        threads.emplace_back([&] {
            for (i32 g = 1; g <= generations; ++g) {
                counter.fetch_add(1);
                barrier.wait();
                if (counter.load() < n * g) violations.fetch_add(1);
            }
        });
    }
    for (std::thread& th : threads) th.join();

    EXPECT_EQ(counter.load(), n * generations);
    EXPECT_EQ(violations.load(), 0);
}

TEST(SpinBarrier, SingleThreadNeverBlocks) {
    SpinBarrier barrier(1);
    barrier.wait();
    barrier.wait();
    SUCCEED();
}

TEST(ThreadCounts, ClusterAndAllPCores) {
    EXPECT_EQ(thread_counts(5, 10), (std::vector<i32>{1, 2, 5, 10}));
}

TEST(ThreadCounts, DedupesAndSorts) {
    EXPECT_EQ(thread_counts(2, 2), (std::vector<i32>{1, 2}));
    EXPECT_EQ(thread_counts(4, 2), (std::vector<i32>{1, 2, 4}));
}

TEST(ThreadCounts, DropsNonPositive) {
    EXPECT_EQ(thread_counts(0, -1), (std::vector<i32>{1, 2}));
}
