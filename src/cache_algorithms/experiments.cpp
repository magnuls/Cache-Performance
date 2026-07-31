#include "experiments.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <limits>
#include <memory>
#include <random>

#include "../system_info/system_info.h"
#include "../types.h"

// Forward decleration for progress bar
static void show_progress(i64 step, i64 total, i64 bytes);

/*
 * We must randomly access array elements so the prefetcher
 * does not automatically pull cache lines for us
 */
const AppleSystemInfo& info = AppleSystemInfo::get_instance();
std::mt19937_64 rng(std::random_device{}());
constexpr i64 kcache_line_size = 128;
volatile Node* dead;
/*
 * First Pass should be dense, go from 4KB -> 256 MB, then detect where
 * the cliffs are and add midpoints there to get a better estimate
 * of the effective cache size. We will double each byte size so
 * it will be 16 doublings to reach 256MB.
 */
struct alignas(kcache_line_size) Node {
    Node* next;
};
static_assert(sizeof(Node) == kcache_line_size);
