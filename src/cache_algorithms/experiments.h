#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H
#include <memory>
#include <random>
#include <vector>

#include "../types.h"

#ifndef __APPLE__
#error "cache_bench targets Apple Silicon"
#endif

/*
 * Sweep bounds. Our starting set is 4KB, our ending set is 256MB.
 * 2^12 = 4096 bytes, 2^28 = 268,435,456 bytes.
 */
constexpr i64 STARTING_SET = i64{1} << 12;
constexpr i64 ENDING_SET = i64{1} << 28;
constexpr i64 TRIALS = 5;

// Random engine thing, defined in experiments.cpp
extern std::mt19937_64 rng;

// Each node is 128 bytes (one cache line). Definition in .cpp.
struct Node;

struct Measurement {
    // Size of the array in bytes
    i64 buffer_bytes;
    f64 ns_per_access;
};

/*
 * sattolo -> shuffles next pointers in place, producing a single
 * cycle through all elements. Works with any T that has a
 * T* next member.
 * Precondition: arr must be identity-linked (arr[i].next == &arr[i]).
 */
template <typename T>
void sattolo(T* arr, i64 count) {
    for (i64 i = 0; i < count - 1; ++i) {
        std::uniform_int_distribution<i64> dist(i + 1, count - 1);
        i64 j = dist(rng);
        std::swap(arr[i].next, arr[j].next);
    }
}

/*
 * warm_loop -> walks the chain untimed once, so cold misses and
 * page faults occur off the clock
 *
 * fill_array -> identity-links (node->next = &node) the array in place
 *
 * total_accesses -> number of accesses for a given array size
 *
 * timed_access -> runs repeated timed chases, returns the
 * Measurement struct with the smallest ns_per_access
 */
void warm_loop(Node* arr, i64 count);
void fill_array(Node* arr, i64 count);
i64 total_accesses(i64 arr_size);
f64 timed_access(Node* arr, i64 num_accesses);

/*
 * Runs the full sweep from STARTING_SET to ENDING_SET.
 * Each Measurement is the minimum over repeated timed runs.
 */
std::vector<Measurement> cache_size_detection();
std::vector<Measurement> cache_line_size_detection();

#endif
