#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "cache_algorithms/config.h"
#include "cache_algorithms/experiments.h"
#include "system_info/system_info.h"

namespace {

i64 steps_between(i64 lo, i64 hi) {
    i64 n = 0;
    for (i64 s = lo; s <= hi; s <<= 1) ++n;
    return n;
}

void expect_doubling_axis(const SweepResult& r, i64 start, i64 end) {
    ASSERT_EQ(static_cast<i64>(r.points.size()), steps_between(start, end));
    for (size_t i = 0; i < r.points.size(); ++i) EXPECT_EQ(r.points[i].x, start << i);
}

void expect_finite_positive(const SweepResult& r) {
    for (const Measurement& m : r.points) {
        EXPECT_TRUE(std::isfinite(m.ns_per_access));
        EXPECT_GT(m.ns_per_access, 0.0);
    }
}

} // namespace

// The read sweep is the slowest part and feeds the write sweep, so run it once.
class SweepTest : public ::testing::Test {
   protected:
    static void SetUpTestSuite() {
        info = std::make_unique<AppleSystemInfo>();
        read = std::make_unique<SweepResult>(cache_size_detection());
    }
    static void TearDownTestSuite() {
        read.reset();
        info.reset();
    }
    static std::unique_ptr<AppleSystemInfo> info;
    static std::unique_ptr<SweepResult> read;
};
std::unique_ptr<AppleSystemInfo> SweepTest::info;
std::unique_ptr<SweepResult> SweepTest::read;

TEST_F(SweepTest, SizeDetectionShape) {
    EXPECT_EQ(read->kind, Sweep::Size);
    EXPECT_EQ(read->axis, Axis::Bytes);
    expect_doubling_axis(*read, STARTING_SET_READ, ENDING_SET_READ);
    expect_finite_positive(*read);
}

TEST_F(SweepTest, LargestWorkingSetIsSlowerThanSmallest) {
    EXPECT_GT(read->points.back().ns_per_access, read->points.front().ns_per_access);
}

TEST_F(SweepTest, LineSizeDetectionShape) {
    SweepResult r = cache_line_size_detection(*info);
    EXPECT_EQ(r.kind, Sweep::LineSize);
    EXPECT_EQ(r.axis, Axis::Stride);
    expect_doubling_axis(r, START_STRIDE_LENGTH, END_STRIDE_LENGTH);
    expect_finite_positive(r);
}

TEST_F(SweepTest, WriteLatencyLinesUpWithReadSweep) {
    SweepResult w = cache_write_latency(read->points);
    EXPECT_EQ(w.kind, Sweep::Write);
    EXPECT_EQ(w.axis, Axis::Bytes);
    ASSERT_EQ(w.points.size(), read->points.size());
    for (size_t i = 0; i < w.points.size(); ++i) {
        EXPECT_EQ(w.points[i].x, read->points[i].x);
        EXPECT_TRUE(std::isfinite(w.points[i].ns_per_access));
        EXPECT_GE(w.points[i].ns_per_access, 0.0);
    }
}
