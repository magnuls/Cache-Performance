#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <gtest/gtest.h>
#include <unistd.h>

#include <filesystem>
#include <set>
#include <string>

#include "cache_algorithms/node.h"
#include "system_info/system_info.h"
#include "types.h"

struct FakeSystemInfo : SystemInfo {
    FakeSystemInfo()
        : SystemInfo(8, i64{1} << 30, 65536, 4194304, -1,
                     kcache_line_size, 16384) {}
    void print_summary() const override {}
};

class TempCwd : public ::testing::Test {
  protected:
    void SetUp() override {
        original_ = std::filesystem::current_path();
        dir_ = std::filesystem::temp_directory_path() /
               ("cache_bench_test_" + std::to_string(getpid()) + "_" +
                std::to_string(counter_++));
        std::filesystem::create_directories(dir_);
        std::filesystem::current_path(dir_);
    }
    void TearDown() override {
        std::filesystem::current_path(original_);
        std::filesystem::remove_all(dir_);
    }
    std::filesystem::path dir_;

  private:
    std::filesystem::path original_;
    static inline int counter_ = 0;
};

//  -1 if a node repeats first
inline i64 walk_cycle(const Node* start) {
    std::set<const Node*> seen;
    const Node* p = start;
    i64 steps = 0;
    do {
        if (!seen.insert(p).second)
            return -1;
        p = p->next;
        ++steps;
    } while (p != start);
    return steps;
}

#endif
