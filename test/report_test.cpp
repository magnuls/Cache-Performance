#include <gtest/gtest.h>

#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "measurement.h"
#include "report/report.h"
#include "test_helpers.h"

namespace {

std::string label(i64 x, Axis axis) {
    char out[8];
    size_label(x, axis, out, sizeof(out));
    return out;
}

std::vector<std::string> read_lines(const std::string& path) {
    std::ifstream in(path);
    std::vector<std::string> lines;
    for (std::string l; std::getline(in, l);) lines.push_back(l);
    return lines;
}

SweepResult one_point(Sweep kind, Axis axis, i64 x = 4096, f64 ns = 1.5) {
    return SweepResult{kind, axis, {Measurement{x, ns}}};
}

} // namespace

TEST(SizeLabel, BytesBelowOneK) {
    EXPECT_EQ(label(0, Axis::Bytes), "0B");
    EXPECT_EQ(label(512, Axis::Bytes), "512B");
    EXPECT_EQ(label(1023, Axis::Bytes), "1023B");
}

TEST(SizeLabel, KibibyteRange) {
    EXPECT_EQ(label(1024, Axis::Bytes), "1K");
    EXPECT_EQ(label(3072, Axis::Bytes), "3K");
    EXPECT_EQ(label(4096, Axis::Bytes), "4K");
}

TEST(SizeLabel, MebibyteRange) {
    EXPECT_EQ(label(i64{1} << 20, Axis::Bytes), "1M");
    EXPECT_EQ(label(i64{3} << 20, Axis::Bytes), "3M");
    EXPECT_EQ(label(i64{1} << 28, Axis::Bytes), "256M");
}

TEST(SizeLabel, StrideSharesTheByteLadder) {
    EXPECT_EQ(label(128, Axis::Stride), "128B");
    EXPECT_EQ(label(2048, Axis::Stride), "2K");
}

TEST(SizeLabel, ThreadsIsABareCount) {
    EXPECT_EQ(label(8, Axis::Threads), "8");
    EXPECT_EQ(label(1024, Axis::Threads), "1024");
}

TEST(SizeLabel, RespectsOutputBufferSize) {
    char out[4];
    std::memset(out, 'x', sizeof(out));
    size_label(1023, Axis::Bytes, out, sizeof(out));
    EXPECT_STREQ(out, "102");
}

TEST(ShowProgress, PrintsStepCountAndLabel) {
    testing::internal::CaptureStderr();
    show_progress(3, 17, 4096, Axis::Bytes);
    std::string err = testing::internal::GetCapturedStderr();
    EXPECT_NE(err.find(" 3/17"), std::string::npos);
    EXPECT_NE(err.find("4K"), std::string::npos);
}

TEST(ShowProgress, BarIsEmptyAtStartAndFullAtEnd) {
    testing::internal::CaptureStderr();
    show_progress(0, 17, 4096, Axis::Bytes);
    std::string start = testing::internal::GetCapturedStderr();
    EXPECT_NE(start.find("[" + std::string(30, ' ') + "]"), std::string::npos);

    testing::internal::CaptureStderr();
    show_progress(17, 17, i64{1} << 28, Axis::Bytes);
    std::string end = testing::internal::GetCapturedStderr();
    EXPECT_NE(end.find("[" + std::string(30, '=') + "]"), std::string::npos);
    EXPECT_NE(end.find("17/17"), std::string::npos);
}

TEST(DisplayMeasurements, HeaderNamesTheAxis) {
    testing::internal::CaptureStdout();
    display_measurements(SweepResult{Sweep::Size, Axis::Bytes, {}});
    EXPECT_NE(testing::internal::GetCapturedStdout().find("size_bytes"), std::string::npos);

    testing::internal::CaptureStdout();
    display_measurements(SweepResult{Sweep::LineSize, Axis::Stride, {}});
    EXPECT_NE(testing::internal::GetCapturedStdout().find("stride_bytes"), std::string::npos);

    testing::internal::CaptureStdout();
    display_measurements(SweepResult{Sweep::Thread, Axis::Threads, {}});
    EXPECT_NE(testing::internal::GetCapturedStdout().find("threads"), std::string::npos);
}

TEST(DisplayMeasurements, OneRowPerPoint) {
    SweepResult r{Sweep::Size, Axis::Bytes, {Measurement{4096, 1.5}, Measurement{8192, 2.25}}};
    testing::internal::CaptureStdout();
    display_measurements(r);
    std::string out = testing::internal::GetCapturedStdout();

    i64 newlines = 0;
    for (char c : out) newlines += (c == '\n');
    EXPECT_EQ(newlines, 3);
    EXPECT_NE(out.find("4096"), std::string::npos);
    EXPECT_NE(out.find("4K"), std::string::npos);
    EXPECT_NE(out.find("8192"), std::string::npos);
    EXPECT_NE(out.find("8K"), std::string::npos);
    EXPECT_NE(out.find("2.25"), std::string::npos);
}

TEST(DisplayMeasurements, EmptySweepPrintsOnlyHeader) {
    testing::internal::CaptureStdout();
    display_measurements(SweepResult{Sweep::Size, Axis::Bytes, {}});
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out.find('\n'), out.size() - 1);
}

class WriteCsv : public TempCwd {};

TEST_F(WriteCsv, FileNameFollowsSweepKind) {
    FakeSystemInfo info;
    write_csv(one_point(Sweep::Size, Axis::Bytes), info);
    write_csv(one_point(Sweep::LineSize, Axis::Stride), info);
    write_csv(one_point(Sweep::Write, Axis::Bytes), info);
    write_csv(one_point(Sweep::Thread, Axis::Bytes), info);
    write_csv(one_point(Sweep::FalseSharing, Axis::Stride), info);
    EXPECT_TRUE(std::filesystem::exists("size_detection.csv"));
    EXPECT_TRUE(std::filesystem::exists("cache_line_size_detection.csv"));
    EXPECT_TRUE(std::filesystem::exists("write_detection.csv"));
    EXPECT_TRUE(std::filesystem::exists("contention.csv"));
    EXPECT_TRUE(std::filesystem::exists("false_sharing.csv"));
}

TEST_F(WriteCsv, HeaderAndRowFormat) {
    FakeSystemInfo info;
    write_csv(one_point(Sweep::Size, Axis::Bytes), info);
    auto lines = read_lines("size_detection.csv");
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], "size_bytes,label,ns_per_access,threads,l1_bytes,l2_bytes,l3_bytes,ram_bytes");
    EXPECT_EQ(lines[1], "4096,4K,1.5,1,65536,4194304,-1,1073741824");
}

TEST_F(WriteCsv, ThreadsColumnCarriesTheCount) {
    FakeSystemInfo info;
    SweepResult r{Sweep::Thread, Axis::Bytes, {Measurement{65536, 2.5, 5}}};
    write_csv(r, info);
    EXPECT_EQ(read_lines("contention.csv")[1], "65536,64K,2.5,5,65536,4194304,-1,1073741824");
}

TEST_F(WriteCsv, FirstColumnFollowsAxis) {
    FakeSystemInfo info;
    write_csv(one_point(Sweep::LineSize, Axis::Stride, 128), info);
    write_csv(one_point(Sweep::Thread, Axis::Threads, 8), info);
    EXPECT_EQ(read_lines("cache_line_size_detection.csv")[0].rfind("stride_bytes,", 0), 0u);
    EXPECT_EQ(read_lines("contention.csv")[0].rfind("threads,", 0), 0u);
}

TEST_F(WriteCsv, EmptySweepWritesHeaderOnly) {
    FakeSystemInfo info;
    write_csv(SweepResult{Sweep::Size, Axis::Bytes, {}}, info);
    EXPECT_EQ(read_lines("size_detection.csv").size(), 1u);
}

TEST_F(WriteCsv, SecondWriteReplacesTheFile) {
    FakeSystemInfo info;
    SweepResult two{Sweep::Size, Axis::Bytes, {Measurement{4096, 1.0}, Measurement{8192, 2.0}}};
    write_csv(two, info);
    write_csv(one_point(Sweep::Size, Axis::Bytes), info);
    EXPECT_EQ(read_lines("size_detection.csv").size(), 2u);
}

TEST_F(WriteCsv, PreservesFifteenSignificantDigits) {
    FakeSystemInfo info;
    write_csv(one_point(Sweep::Size, Axis::Bytes, 4096, 1.23456789012345), info);
    EXPECT_NE(read_lines("size_detection.csv")[1].find("1.23456789012345"), std::string::npos);
}

TEST_F(WriteCsv, DetectionCsvHasOneRowPerQuantity) {
    FakeSystemInfo info;
    write_detection_csv(Detected{131072, 155776, 13000000, 16777216, 128}, info);
    auto lines = read_lines("detection.csv");
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[0], "quantity,detected_bytes,next_bytes,spec_bytes");
    EXPECT_EQ(lines[1], "l1d,131072,155776,65536");
    EXPECT_EQ(lines[2], "l2,13000000,16777216,4194304");
    EXPECT_EQ(lines[3], "line_size,128,-1,128");
}

TEST(PrintDetection, ShowsLabelsAndError) {
    FakeSystemInfo info;
    testing::internal::CaptureStdout();
    print_detection(Detected{65536, 131072, -1, -1, 128}, info);
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("l1d"), std::string::npos);
    EXPECT_NE(out.find("64K"), std::string::npos);
    EXPECT_NE(out.find("1.00x"), std::string::npos);
    EXPECT_NE(out.find("n/a"), std::string::npos);
    EXPECT_NE(out.find("128B"), std::string::npos);
}
