#include <gtest/gtest.h>

#include <vector>

#include "analysis/detect.h"
#include "measurement.h"

namespace {

std::vector<Measurement> curve(std::vector<f64> ns, i64 start = 4096) {
    std::vector<Measurement> pts;
    i64 x = start;
    for (f64 v : ns) {
        pts.push_back(Measurement{x, v});
        x <<= 1;
    }
    return pts;
}

SweepResult sweep(Sweep kind, Axis axis, std::vector<Measurement> pts) {
    return SweepResult{kind, axis, std::move(pts)};
}

} // namespace

TEST(FindCliffs, ThreePlateausGiveTwoCliffs) {
    auto pts = curve({1, 1, 1, 1, 6, 6, 6, 6, 20, 40, 70, 100, 100});
    auto cliffs = find_cliffs(pts, 1.4);
    ASSERT_EQ(cliffs.size(), 2u);
    EXPECT_EQ(cliffs[0].before, 4096 << 3);
    EXPECT_EQ(cliffs[0].after, 4096 << 4);
    EXPECT_DOUBLE_EQ(cliffs[0].ratio, 6.0);
    EXPECT_EQ(cliffs[1].before, 4096 << 7);
    EXPECT_EQ(cliffs[1].after, 4096 << 8);
}

TEST(FindCliffs, FlatCurveHasNone) {
    EXPECT_TRUE(find_cliffs(curve({5, 5.1, 4.9, 5, 5.05}), 1.4).empty());
}

TEST(FindCliffs, EqualRatioRampCountsOnce) {
    EXPECT_EQ(find_cliffs(curve({1, 2, 4, 8}), 1.4).size(), 1u);
}

TEST(FindCliffs, DecreasingStepsAfterPeakAreNotCliffs) {
    auto cliffs = find_cliffs(curve({8, 20, 43, 87, 103, 105}), 1.4);
    ASSERT_EQ(cliffs.size(), 1u);
    EXPECT_EQ(cliffs[0].before, 4096);
}

TEST(FindCliffs, IgnoresNonPositiveLatency) {
    EXPECT_TRUE(find_cliffs(curve({0, 5, 0, 5}), 1.4).empty());
}

TEST(FindCliffs, TooFewPointsGiveNone) {
    EXPECT_TRUE(find_cliffs(curve({1}), 1.4).empty());
    EXPECT_TRUE(find_cliffs({}, 1.4).empty());
}

TEST(Midpoints, GeometricInsideAlignedAscending) {
    const i64 lo = i64{8} << 20, hi = i64{16} << 20;
    auto mids = midpoints(lo, hi, 3, 128);
    ASSERT_EQ(mids.size(), 3u);
    for (size_t i = 0; i < mids.size(); ++i) {
        EXPECT_GT(mids[i], lo);
        EXPECT_LT(mids[i], hi);
        EXPECT_EQ(mids[i] % 128, 0);
        if (i > 0) EXPECT_GT(mids[i], mids[i - 1]);
    }
    EXPECT_NEAR(static_cast<f64>(mids[1]), lo * 1.4142, 256.0);
}

TEST(Midpoints, DedupesWhenGranularityIsCoarse) {
    auto mids = midpoints(128, 256, 8, 128);
    EXPECT_TRUE(mids.empty());
    auto few = midpoints(1024, 2048, 8, 512);
    ASSERT_EQ(few.size(), 1u);
    EXPECT_EQ(few[0], 1536);
}

TEST(Midpoints, BadArgumentsGiveNone) {
    EXPECT_TRUE(midpoints(16, 8, 3, 1).empty());
    EXPECT_TRUE(midpoints(8, 8, 3, 1).empty());
    EXPECT_TRUE(midpoints(8, 16, 0, 1).empty());
    EXPECT_TRUE(midpoints(0, 16, 3, 1).empty());
}

TEST(DetectGeometry, ReadsL1AndL2FromSizeSweep) {
    auto size = sweep(Sweep::Size, Axis::Bytes,
                      curve({0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 6, 6, 6, 6, 6,
                             7, 8, 20, 44, 87, 103, 105}));
    auto fs = sweep(Sweep::FalseSharing, Axis::Stride, {});
    Detected d = detect_geometry(size, fs, 1.4);
    EXPECT_EQ(d.l1d, 4096 << 5);
    EXPECT_EQ(d.l1d_next, 4096 << 6);
    EXPECT_EQ(d.l2, 4096 << 12);
    EXPECT_EQ(d.l2_next, 4096 << 13);
    EXPECT_EQ(d.line_size, -1);
}

TEST(DetectGeometry, L2IsThePlateauEndNotTheBiggestLaterStep) {
    auto size = sweep(Sweep::Size, Axis::Bytes,
                      curve({1, 1, 1, 10, 10, 10, 22, 60, 100, 100}));
    Detected d = detect_geometry(
        size, sweep(Sweep::FalseSharing, Axis::Stride, {}), 1.4);
    EXPECT_EQ(d.l1d, 4096 << 2);
    EXPECT_EQ(d.l2, 4096 << 5);
    EXPECT_EQ(d.l2_next, 4096 << 6);
}

TEST(DetectGeometry, L2SearchSkipsTheWholeL1CliffRun) {
    auto size = sweep(Sweep::Size, Axis::Bytes,
                      curve({1, 1, 4, 8, 8, 8, 8, 30, 30}));
    Detected d = detect_geometry(
        size, sweep(Sweep::FalseSharing, Axis::Stride, {}), 1.4);
    EXPECT_EQ(d.l1d, 4096 << 1);
    EXPECT_EQ(d.l2, 4096 << 6);
}

TEST(DetectGeometry, UsesRefinedPointsInsideACliff) {
    auto pts = curve({1, 1, 1, 10, 10});
    pts.push_back(Measurement{4096 * 5, 1.1});
    pts.push_back(Measurement{4096 * 6, 8.0});
    pts.push_back(Measurement{4096 * 7, 9.5});
    std::sort(pts.begin(), pts.end(),
              [](const Measurement& a, const Measurement& b) {
                  return a.x < b.x;
              });
    auto size = sweep(Sweep::Size, Axis::Bytes, pts);
    Detected d = detect_geometry(
        size, sweep(Sweep::FalseSharing, Axis::Stride, {}), 1.4);
    EXPECT_EQ(d.l1d, 4096 * 5);
    EXPECT_EQ(d.l1d_next, 4096 * 6);
    EXPECT_EQ(d.l2, -1);
    EXPECT_EQ(d.l2_next, -1);
}

TEST(DetectGeometry, LineSizeIsFirstSeparationAfterTheDrop) {
    std::vector<Measurement> fs;
    i64 sep = 8;
    for (f64 ns : {30.0, 31.0, 29.0, 30.0, 1.0, 1.0, 1.0, 1.0}) {
        fs.push_back(Measurement{sep, ns, 2});
        sep <<= 1;
    }
    Detected d = detect_geometry(
        sweep(Sweep::Size, Axis::Bytes, {}),
        sweep(Sweep::FalseSharing, Axis::Stride, fs), 1.4);
    EXPECT_EQ(d.line_size, 128);
    EXPECT_EQ(d.l1d, -1);
    EXPECT_EQ(d.l2, -1);
}

TEST(DetectGeometry, NoDropMeansNoLineSize) {
    std::vector<Measurement> fs;
    i64 sep = 8;
    for (int i = 0; i < 8; ++i) {
        fs.push_back(Measurement{sep, 1.0, 2});
        sep <<= 1;
    }
    Detected d = detect_geometry(
        sweep(Sweep::Size, Axis::Bytes, {}),
        sweep(Sweep::FalseSharing, Axis::Stride, fs), 1.4);
    EXPECT_EQ(d.line_size, -1);
}
