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

void fill_array(Node* arr, i64 count) {
    for (i64 i{}; i < count; ++i) {
        arr[i] = Node();
        arr[i].next = &arr[i];
    }
}

void warm_loop(Node* arr, i64 count) {
    Node* temp = &arr[0];
    while (--count >= 0) {
        temp = temp->next;
    }
    dead = temp;
}

i64 total_accesses(i64 arr_size) {
    i64 passes = 10;
    i64 k_min = 10000000, k_max = 50000000;
    return std::clamp(passes * arr_size, k_min, k_max);
}
// clamp(v,lo,hi) returns
// lo if v < lo,
// hi if v > hi,
// else v

f64 timed_access(Node* arr, i64 num_accesses) {
    Node* p = arr;
    const auto start{std::chrono::steady_clock::now()};
    for (i64 i = 0; i < num_accesses; ++i) p = p->next;
    const auto finish{std::chrono::steady_clock::now()};
    // volatile assignment so compiler dosen't frick me
    dead = p;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start)
               .count() /
           static_cast<f64>(num_accesses);
}
