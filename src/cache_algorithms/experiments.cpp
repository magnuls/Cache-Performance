#include "cache_algorithms/experiments.h"

#include <cassert>

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <unistd.h>

#include "cache_algorithms/config.h"
#include "cache_algorithms/node.h"
#include "cache_algorithms/sattolo.h"
#include "report/report.h"
#include "system_info/system_info.h"
#include "types.h"

/*
 * First Pass should be dense, go from 4KB -> 256 MB, then detect where
 * the cliffs are and add midpoints there to get a better estimate
 * of the effective cache size. We will double each byte size so
 * it will be 16 doublings to reach 256MB.
 */

namespace {

/*
 * fill_array -> identity-links (node->next = &node) the array in place
 */
void fill_array(Node* arr, i64 count) {
    for (i64 i{}; i < count; ++i) {
        arr[i] = Node();
        arr[i].next = &arr[i];
    }
}

/*
 * warm_loop -> walks the chain untimed once, so cold misses and
 * page faults occur off the clock
 */
void warm_loop(Node* arr, i64 count) {
    Node* temp = &arr[0];
    while (--count >= 0) {
        temp = temp->next;
    }
    asm volatile("" ::"r"(temp) : "memory");
}

/*
 * total_accesses -> number of accesses for a given array size
 */
i64 total_accesses(i64 arr_size) {
    i64 passes = 10;
    i64 k_min = 10000000, k_max = 50000000;
    return std::clamp(passes * arr_size, k_min, k_max);
}

/*
 * timed_access -> runs repeated timed chases, returns the
 * Measurement struct with the smallest ns_per_access
 */
f64 timed_access(Node* arr, i64 num_accesses) {
    Node* volatile dead;
    Node* p = arr;
    const auto start{std::chrono::steady_clock::now()};
    for (i64 i = 0; i < num_accesses; ++i)
        p = p->next;
    const auto finish{std::chrono::steady_clock::now()};
    // volatile assignment so compiler dosen't frick me
    dead = p;
    asm volatile("" ::"r"(dead) : "memory");
    return std::chrono::duration<f64, std::nano>(finish - start).count() /
           static_cast<f64>(num_accesses);
}

} // namespace

/*
 * Sequence of operations...
 * Instantiate array size -> Create Array -> fill array -> shuffle -> warm loop
 * -> timed_acess -> push to measurements -> back to beginning
 */
SweepResult cache_size_detection() {
    std::vector<Measurement> measurements;
    i64 total_steps = 0;
    for (i64 s = STARTING_SET_READ; s <= ENDING_SET_READ; s <<= 1)
        ++total_steps;
    i64 step = 0;

    for (i64 i = STARTING_SET_READ; i <= ENDING_SET_READ; i <<= 1) {
        // Rng device
        std::mt19937_64 rng(std::random_device{}());
        // Put the progress bar for every size
        show_progress(step++, total_steps, i, Axis::Bytes);
        i64 count = i / sizeof(Node);
        std::unique_ptr<Node[]> arr = std::make_unique<Node[]>(count);
        fill_array(arr.get(), count);
        sattolo(arr.get(), count, rng);
        warm_loop(arr.get(), count);

        i64 accesses = total_accesses(count);
        f64 min_ns_pa = std::numeric_limits<f64>::max();
        for (i16 t{}; t < TRIALS; ++t) {
            min_ns_pa = std::min(timed_access(arr.get(), accesses), min_ns_pa);
        }
        measurements.push_back(Measurement{static_cast<i64>(sizeof(Node)) * count, min_ns_pa});
    }
    show_progress(total_steps, total_steps, ENDING_SET_READ, Axis::Bytes);
    std::fprintf(stderr, "\n");
    return SweepResult{Sweep::Size, Axis::Bytes, std::move(measurements)};
}

SweepResult cache_line_size_detection(const AppleSystemInfo& s) {
    assert(START_STRIDE_LENGTH >= sizeof(u32));

    // Constant across the sweep: enough slots that at stride >= line size the
    // touched-line footprint is 4 x L2, keeping the plateau region out of cache.
    const i64 num_slots = (4 * s.l2_cache) / kcache_line_size;
    std::vector<Measurement> measurements;
    std::mt19937_64 rng(std::random_device{}());

    // Pointer Chasing
    auto chase = [&](const i64 n, const i64 words_per_slot, const std::vector<u32>& buf) -> f64 {
        u32 cur = 0;
        auto start = std::chrono::steady_clock::now();

        for (i64 j = 0; j < n; ++j)
            cur = buf[static_cast<i64>(cur) * words_per_slot];

        auto end = std::chrono::steady_clock::now();

        /*
         * asm -> raw assembly instructions
         *
         * "r"(cur) means that asm reads cur from a register
         * so we must have cur be a computed value at this Pointer
         *
         * "memory" -> asm might read or write to memory so the compiler
         * can't reorder memory ops accros it or cache values across the
         * boundry
         *
         * volatile -> don't delete this asm even though basically nothing happens
         * to it
         */
        asm volatile("" ::"r"(cur) : "memory");
        return std::chrono::duration<f64, std::nano>(end - start).count() / n;
    };

    i64 total_steps{}, step{};

    for (i64 _ = START_STRIDE_LENGTH; _ <= END_STRIDE_LENGTH; _ <<= 1)
        ++total_steps;

    for (i64 stride = START_STRIDE_LENGTH; stride <= END_STRIDE_LENGTH; stride <<= 1) {
        show_progress(step++, total_steps, stride, Axis::Stride);

        const i64 buffer_bytes = num_slots * stride;
        std::vector<u32> buf(buffer_bytes / sizeof(u32), 0);
        const i64 words_per_slot = stride / sizeof(u32);

        // Sattolo: bake the successor table into the buffer at slot boundaries
        std::vector<u32> next = sattolo_cycle(num_slots, rng);
        for (i64 i = 0; i < num_slots; ++i) {
            buf[i * words_per_slot] = next[i];
        }
        // Warm loop
        chase(num_slots, words_per_slot, buf);
        // Real loop
        f64 min_ns = std::numeric_limits<f64>::max();
        for (i64 t = 0; t < TRIALS; ++t) {
            min_ns = std::min(min_ns, chase(num_slots, words_per_slot, buf));
        }
        measurements.push_back({stride, min_ns});
    }
    show_progress(total_steps, total_steps, END_STRIDE_LENGTH, Axis::Stride);
    std::fprintf(stderr, "\n");
    return SweepResult{Sweep::LineSize, Axis::Stride, std::move(measurements)};
}
