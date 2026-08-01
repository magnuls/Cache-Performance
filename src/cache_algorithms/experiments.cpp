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

/*
 * Sequence of operations...
 * Instantiate array size -> Create Array -> fill array -> shuffle -> warm loop
 * -> timed_acess -> push to measurements -> back to beginning
 */
std::vector<Measurement> cache_size_detection() {
    std::vector<Measurement> measurements;
    i64 total_steps = 0;
    for (i64 s = STARTING_SET; s <= ENDING_SET; s <<= 1) ++total_steps;
    i64 step = 0;

    for (i64 i = STARTING_SET; i <= ENDING_SET; i <<= 1) {
        // Put the progress bar for every size
        show_progress(step++, total_steps, i);
        i64 count = i / sizeof(Node);
        std::unique_ptr<Node[]> arr = std::make_unique<Node[]>(count);
        fill_array(arr.get(), count);
        sattolo(arr.get(), count);
        warm_loop(arr.get(), count);

        i64 accesses = total_accesses(count);
        f64 min_ns_pa = std::numeric_limits<f64>::max();
        for (i16 i{}; i < TRIALS; ++i) {
            min_ns_pa = std::min(timed_access(arr.get(), accesses), min_ns_pa);
        }
        measurements.push_back(
            Measurement{static_cast<i64>(sizeof(Node)) * count, min_ns_pa});
    }
    show_progress(total_steps, total_steps, ENDING_SET);
    std::fprintf(stderr, "\n");
    return measurements;
}

// Needs implementation
std::vector<Measurement> cache_line_size_detection();

void size_label(i64 bytes, char* out, size_t out_size) {
    f64 kib = bytes / 1024.0f;
    if (kib < 1024.0f) {
        // Rounds numbers btw
        std::snprintf(out, out_size, "%.0fK", kib);
    } else {
        std::snprintf(out, out_size, "%.0fM", kib / 1024.0f);
    }
}

/*
 * Progress goes to stderr so ./cache_bench > results.csv keeps the
 * CSV clean. '\r' rewrites the same terminal line in place
 * the caller prints the final newline once the sweep is done.
 */
static void show_progress(i64 step, i64 total, i64 bytes) {
    constexpr i64 kbar_width = 30;
    i32 filled = static_cast<i32>(step * kbar_width / total);
    char label[8];
    size_label(bytes, label, sizeof(label));
    std::fprintf(stderr, "\r[%-*.*s] %2lld/%lld  %5s",
                 static_cast<i32>(kbar_width), filled,
                 "==============================", static_cast<long long>(step),
                 static_cast<long long>(total), label);
    std::fflush(stderr);
}

/*
 * Aligned table to stdout for output in the terminal
 */
void display_measurements(const std::vector<Measurement>& v) {
    std::printf("%10s, %5s, %18s\n", "size_bytes", "label", "ns_per_access");
    for (const Measurement& m : v) {
        char label[8];
        size_label(m.buffer_bytes, label, sizeof(label));
        std::printf("%10lld, %5s, %18.15g\n",
                    static_cast<long long>(m.buffer_bytes), label,
                    m.ns_per_access);
    }
}

/*
 *
 * Writes to a csv file.
 */
void write_csv(const std::vector<Measurement>& v, const char* path) {
    FILE* f = std::fopen(path, "w");
    if (!f) {
        std::fprintf(stderr, "write_csv: could not open %s\n", path);
        return;
    }
    std::fprintf(f, "size_bytes,label,ns_per_access\n");
    for (const Measurement& m : v) {
        char label[8];
        size_label(m.buffer_bytes, label, sizeof(label));
        // Double is accurate to 15 significant digits, %g counts those
        std::fprintf(f, "%lld,%s,%.15g\n",
                     static_cast<long long>(m.buffer_bytes), label,
                     m.ns_per_access);
    }
    std::fclose(f);
}
