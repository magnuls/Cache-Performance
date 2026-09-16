#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "cache_algorithms/node.h"
#include "system_info/system_info.h"

// One instance for the file: the constructor shells out to system_profiler.
class AppleSystemInfoTest : public ::testing::Test {
   protected:
    static void SetUpTestSuite() { info = std::make_unique<AppleSystemInfo>(); }
    static void TearDownTestSuite() { info.reset(); }
    static std::unique_ptr<AppleSystemInfo> info;
};
std::unique_ptr<AppleSystemInfo> AppleSystemInfoTest::info;

TEST_F(AppleSystemInfoTest, CoreCountsAddUp) {
    EXPECT_GT(info->core_count, 0);
    EXPECT_GT(info->p_cores, 0);
    EXPECT_EQ(info->p_cores + info->e_cores, info->core_count);
}

TEST_F(AppleSystemInfoTest, CacheLineMatchesCompileTimeConstant) {
    EXPECT_EQ(info->cache_line_size, kcache_line_size);
}

TEST_F(AppleSystemInfoTest, PageSizeIsPowerOfTwo) {
    EXPECT_GE(info->page_size, 4096);
    EXPECT_EQ(info->page_size & (info->page_size - 1), 0);
}

TEST_F(AppleSystemInfoTest, CacheHierarchyGrows) {
    EXPECT_GT(info->l1_cache, 0);
    EXPECT_LT(info->l1_cache, info->l2_cache);
    EXPECT_LT(info->l2_cache, info->total_ram);
    EXPECT_EQ(info->p_l1d_cache, info->l1_cache);
}

TEST_F(AppleSystemInfoTest, ClustersDivideCoresEvenly) {
    EXPECT_GT(info->p_cpus_per_l2, 0);
    EXPECT_EQ(info->p_clusters * info->p_cpus_per_l2, info->p_cores);
    if (info->e_cores > 0) {
        EXPECT_GT(info->e_cpus_per_l2, 0);
        EXPECT_EQ(info->e_clusters * info->e_cpus_per_l2, info->e_cores);
    }
}

TEST_F(AppleSystemInfoTest, SystemLevelCacheIsNotExposed) {
    EXPECT_EQ(info->l3_cache, -1);
    EXPECT_EQ(info->slc_bytes, -1);
}

TEST_F(AppleSystemInfoTest, ChipNameIsKnown) {
    EXPECT_FALSE(info->chip_name.empty());
    EXPECT_NE(info->chip_name, "unknown");
}

TEST_F(AppleSystemInfoTest, SummaryPrintsHeaderAndChip) {
    testing::internal::CaptureStdout();
    info->print_summary();
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("Apple Silicon System Info"), std::string::npos);
    EXPECT_NE(out.find("Chip:"), std::string::npos);
    EXPECT_NE(out.find(info->chip_name), std::string::npos);
}
