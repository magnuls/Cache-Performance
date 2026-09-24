#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <vector>

#include "cache_algorithms/config.h"
#include "analysis/detect.h"
#include "cache_algorithms/experiments.h"
#include "cache_algorithms/threads.h"
#include "system_info/system_info.h"

namespace {

i64 steps_between(i64 lo, i64 hi) {
    i64 n = 0;
    for (i64 s = lo; s <= hi; s <<= 1)
        ++n;
    return n;
}

void expect_doubling_axis(const SweepResult& r, i64 start, i64 end) {
    ASSERT_EQ(static_cast<i64>(r.points.size()),
              steps_between(start, end));
    for (size_t i = 0; i < r.points.size(); ++i)
        EXPECT_EQ(r.points[i].x, start << i);
}

void expect_finite_positive(const SweepResult& r) {
    for (const Measurement& m : r.points) {
        EXPECT_TRUE(std::isfinite(m.ns_per_access));
        EXPECT_GT(m.ns_per_access, 0.0);
    }
}

} // namespace

// run once
class SweepTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        info = std::make_unique<AppleSystemInfo>();
        read = std::make_unique<SweepResult>(cache_size_detection());
        refined = std::make_unique<SweepResult>(*read);
        refine_size_sweep(*refined);
        false_sharing =
            std::make_unique<SweepResult>(cache_false_sharing());
    }
    static void TearDownTestSuite() {
        false_sharing.reset();
        refined.reset();
        read.reset();
        info.reset();
    }
    static std::unique_ptr<AppleSystemInfo> info;
    static std::unique_ptr<SweepResult> read;
    static std::unique_ptr<SweepResult> refined;
    static std::unique_ptr<SweepResult> false_sharing;
};
std::unique_ptr<AppleSystemInfo> SweepTest::info;
std::unique_ptr<SweepResult> SweepTest::read;
std::unique_ptr<SweepResult> SweepTest::refined;
std::unique_ptr<SweepResult> SweepTest::false_sharing;

TEST_F(SweepTest, SizeDetectionShape) {
    EXPECT_EQ(read->kind, Sweep::Size);
    EXPECT_EQ(read->axis, Axis::Bytes);
    expect_doubling_axis(*read, STARTING_SET_READ, ENDING_SET_READ);
    expect_finite_positive(*read);
}

TEST_F(SweepTest, LargestWorkingSetIsSlowerThanSmallest) {
    EXPECT_GT(read->points.back().ns_per_access,
              read->points.front().ns_per_access);
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

TEST_F(SweepTest, WriteLatencyAcceptsRefinedReadSweep) {
    SweepResult w = cache_write_latency(refined->points);
    EXPECT_EQ(w.points.size(), read->points.size());
}

TEST_F(SweepTest, RefinementAddsSortedUniquePoints) {
    EXPECT_GT(refined->points.size(), read->points.size());
    for (size_t i = 1; i < refined->points.size(); ++i)
        EXPECT_LT(refined->points[i - 1].x, refined->points[i].x);
    for (const Measurement& m : read->points) {
        bool present = std::any_of(
            refined->points.begin(), refined->points.end(),
            [&](const Measurement& r) { return r.x == m.x; });
        EXPECT_TRUE(present);
    }
    expect_finite_positive(*refined);
}

TEST_F(SweepTest, FalseSharingShape) {
    EXPECT_EQ(false_sharing->kind, Sweep::FalseSharing);
    EXPECT_EQ(false_sharing->axis, Axis::Stride);
    expect_doubling_axis(*false_sharing, START_SEPARATION, END_SEPARATION);
    expect_finite_positive(*false_sharing);
    for (const Measurement& m : false_sharing->points)
        EXPECT_EQ(m.threads, 2);
}

TEST_F(SweepTest, SharedLineIsSlowerThanSeparateLines) {
    EXPECT_GT(false_sharing->points.front().ns_per_access,
              2.0 * false_sharing->points.back().ns_per_access);
}

TEST_F(SweepTest, ContentionShape) {
    SweepResult c = cache_contention(*info);
    EXPECT_EQ(c.kind, Sweep::Thread);
    EXPECT_EQ(c.axis, Axis::Bytes);
    expect_finite_positive(c);

    const std::vector<i32> counts =
        thread_counts(info->p_cpus_per_l2, info->p_cores);
    const i64 sizes =
        steps_between(STARTING_SET_CONTENTION, ENDING_SET_CONTENTION);
    ASSERT_EQ(static_cast<i64>(c.points.size()),
              static_cast<i64>(counts.size()) * sizes);

    std::set<i32> seen;
    for (const Measurement& m : c.points) seen.insert(m.threads);
    EXPECT_EQ(std::vector<i32>(seen.begin(), seen.end()), counts);

    for (i32 t : counts) {
        i64 expected = STARTING_SET_CONTENTION;
        for (const Measurement& m : c.points) {
            if (m.threads != t) continue;
            EXPECT_EQ(m.x, expected);
            expected <<= 1;
        }
        EXPECT_EQ(expected, ENDING_SET_CONTENTION << 1);
    }
}

TEST_F(SweepTest, DetectsThisMachinesGeometry) {
    Detected d = detect_geometry(*refined, *false_sharing);
    EXPECT_EQ(d.l1d, info->l1_cache);
    EXPECT_GE(d.line_size, 32);
    EXPECT_LE(d.line_size, info->cache_line_size);
    EXPECT_EQ(info->cache_line_size % d.line_size, 0);
    EXPECT_GE(d.l2, info->l2_cache / 2);
    EXPECT_LE(d.l2, info->l2_cache);
}
