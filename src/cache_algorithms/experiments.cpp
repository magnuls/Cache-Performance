#include "cache_algorithms/experiments.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <limits>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include "analysis/detect.h"
#include "cache_algorithms/chase.h"
#include "cache_algorithms/config.h"
#include "cache_algorithms/node.h"
#include "cache_algorithms/sattolo.h"
#include "cache_algorithms/threads.h"
#include "report/report.h"
#include "system_info/system_info.h"
#include "types.h"

namespace {

i64 doubling_steps(i64 lo, i64 hi) {
    i64 n = 0;
    for (i64 s = lo; s <= hi; s <<= 1) ++n;
    return n;
}

f64 elapsed_ns(std::chrono::steady_clock::time_point start,
               std::chrono::steady_clock::time_point end) {
    return std::chrono::duration<f64, std::nano>(end - start).count();
}

struct alignas(kcache_line_size) Line {
    std::atomic<u64> w[kcache_line_size / sizeof(u64)];
};

} // namespace

/*
 * First Pass should be dense, go from 4KB -> 256 MB, then detect
 * where the cliffs are and add midpoints there to get a better
 * estimate of the effective cache size. We will double each byte size
 * so it will be 16 doublings to reach 256MB.
 */

/*
 * Sequence of operations:
 * Instantiate array size -> Create Array -> fill array -> shuffle ->
 * warm loop
 * -> timed_acess -> push to measurements -> back to beginning
 */
Measurement measure_working_set(i64 bytes) {
    std::mt19937_64 rng(std::random_device{}());
    const i64 count = bytes / sizeof(Node);
    std::unique_ptr<Node[]> arr = std::make_unique<Node[]>(count);
    fill_array(arr.get(), count);
    sattolo(arr.get(), count, rng);
    warm_loop(arr.get(), count);

    const i64 accesses = total_accesses(count);
    f64 min_ns_pa = std::numeric_limits<f64>::max();
    for (i16 t{}; t < TRIALS; ++t) {
        min_ns_pa = std::min(timed_access(arr.get(), accesses), min_ns_pa);
    }
    return Measurement{static_cast<i64>(sizeof(Node)) * count, min_ns_pa};
}

SweepResult cache_size_detection() {
    std::vector<Measurement> measurements;
    const i64 total_steps =
        doubling_steps(STARTING_SET_READ, ENDING_SET_READ);
    i64 step = 0;

    for (i64 i = STARTING_SET_READ; i <= ENDING_SET_READ; i <<= 1) {
        show_progress(step++, total_steps, i, Axis::Bytes);
        measurements.push_back(measure_working_set(i));
    }
    show_progress(total_steps, total_steps, ENDING_SET_READ,
                  Axis::Bytes);
    std::fprintf(stderr, "\n");
    return SweepResult{Sweep::Size, Axis::Bytes,
                       std::move(measurements)};
}

void refine_size_sweep(SweepResult& r) {
    std::vector<i64> sizes;
    for (const Cliff& c : find_cliffs(r.points, CLIFF_MIN_RATIO))
        for (i64 s : midpoints(c.before, c.after, REFINE_POINTS,
                               sizeof(Node)))
            sizes.push_back(s);

    const i64 total_steps = static_cast<i64>(sizes.size());
    i64 step = 0;
    for (i64 s : sizes) {
        show_progress(step++, total_steps, s, Axis::Bytes);
        r.points.push_back(measure_working_set(s));
    }
    if (total_steps > 0) {
        show_progress(total_steps, total_steps, sizes.back(),
                      Axis::Bytes);
        std::fprintf(stderr, "\n");
    }

    std::sort(r.points.begin(), r.points.end(),
              [](const Measurement& a, const Measurement& b) {
                  return a.x < b.x;
              });
    r.points.erase(std::unique(r.points.begin(), r.points.end(),
                               [](const Measurement& a,
                                  const Measurement& b) {
                                   return a.x == b.x;
                               }),
                   r.points.end());
}

SweepResult cache_line_size_detection(const AppleSystemInfo& s) {
    assert(START_STRIDE_LENGTH >= sizeof(u32));

    // Constant across the sweep: enough slots that at stride >= line
    // size the touched-line footprint is 4 x L2, keeping the plateau
    // region out of cache.
    const i64 num_slots = (4 * s.l2_cache) / kcache_line_size;
    std::vector<Measurement> measurements;
    std::mt19937_64 rng(std::random_device{}());

    // Pointer Chasing
    auto chase = [&](const i64 n, const i64 words_per_slot,
                     const std::vector<u32>& buf) -> f64 {
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
         * "memory" -> asm might read or write to memory so the
         * compiler can't reorder memory ops accros it or cache values
         * across the boundry
         *
         * volatile -> don't delete this asm even though basically
         * nothing happens to it
         */
        asm volatile("" ::"r"(cur) : "memory");
        return elapsed_ns(start, end) / n;
    };

    const i64 total_steps =
        doubling_steps(START_STRIDE_LENGTH, END_STRIDE_LENGTH);
    i64 step = 0;

    for (i64 stride = START_STRIDE_LENGTH;
         stride <= END_STRIDE_LENGTH; stride <<= 1) {
        show_progress(step++, total_steps, stride, Axis::Stride);

        const i64 buffer_bytes = num_slots * stride;
        std::vector<u32> buf(buffer_bytes / sizeof(u32), 0);
        const i64 words_per_slot = stride / sizeof(u32);

        // Sattolo: bake the successor table into the buffer at slot
        // boundaries
        std::vector<u32> next = sattolo_cycle(num_slots, rng);
        for (i64 i = 0; i < num_slots; ++i) {
            buf[i * words_per_slot] = next[i];
        }
        // Warm loop
        chase(num_slots, words_per_slot, buf);
        // Real loop
        f64 min_ns = std::numeric_limits<f64>::max();
        for (i64 t = 0; t < TRIALS; ++t) {
            min_ns = std::min(min_ns,
                              chase(num_slots, words_per_slot, buf));
        }
        measurements.push_back({stride, min_ns});
    }
    show_progress(total_steps, total_steps, END_STRIDE_LENGTH,
                  Axis::Stride);
    std::fprintf(stderr, "\n");
    return SweepResult{Sweep::LineSize, Axis::Stride,
                       std::move(measurements)};
}

// We need the difference between (read+write) and read to get the
// write measurement timing read_measurements will be the vector that
// holds Measurements for the read only
SweepResult cache_write_latency(
    const std::vector<Measurement>& read_measurements) {
    std::vector<Measurement> measurements;
    const i64 total_steps =
        doubling_steps(STARTING_SET_WRITE, ENDING_SET_WRITE);
    i64 step = 0;
    for (i64 i = STARTING_SET_WRITE; i <= ENDING_SET_WRITE; i <<= 1) {
        std::mt19937_64 rng(std::random_device{}());

        show_progress(step++, total_steps, i, Axis::Bytes);
        i64 count = i / sizeof(Node);

        std::unique_ptr<Node[]> arr = std::make_unique<Node[]>(count);
        fill_array(arr.get(), count);
        sattolo(arr.get(), count, rng);
        warm_loop(arr.get(), count);

        i64 accesses = total_accesses(count);
        f64 min_ns_pa = std::numeric_limits<f64>::max();
        for (i16 t{}; t < TRIALS; ++t) {
            min_ns_pa = std::min(
                timed_access(arr.get(), accesses, Sweep::Write),
                min_ns_pa);
        }

        const i64 bytes = static_cast<i64>(sizeof(Node)) * count;
        auto read = std::find_if(
            read_measurements.begin(), read_measurements.end(),
            [bytes](const Measurement& m) { return m.x == bytes; });
        assert(read != read_measurements.end());
        measurements.push_back(Measurement{
            bytes, std::max(min_ns_pa - read->ns_per_access, 0.0)});
    }
    show_progress(total_steps, total_steps, ENDING_SET_WRITE,
                  Axis::Bytes);
    std::fprintf(stderr, "\n");
    return SweepResult{Sweep::Write, Axis::Bytes,
                       std::move(measurements)};
}

SweepResult cache_contention(const AppleSystemInfo& s) {
    std::vector<Measurement> measurements;
    const std::vector<i32> counts = thread_counts(s.p_cpus_per_l2, s.p_cores);
    const i64 total_steps =
        static_cast<i64>(counts.size()) *
        doubling_steps(STARTING_SET_CONTENTION, ENDING_SET_CONTENTION);
    i64 step = 0;

    for (i32 t : counts) {
        for (i64 bytes = STARTING_SET_CONTENTION;
             bytes <= ENDING_SET_CONTENTION; bytes <<= 1) {
            show_progress(step++, total_steps, bytes, Axis::Bytes);

            SpinBarrier barrier(t);
            std::vector<f64> per_thread(t, 0.0);
            std::vector<std::thread> workers;
            for (i32 w = 0; w < t; ++w) {
                workers.emplace_back([&, w] {
                    pin_to_p_cores();
                    std::mt19937_64 rng(std::random_device{}());
                    const i64 count = bytes / sizeof(Node);
                    std::unique_ptr<Node[]> arr =
                        std::make_unique<Node[]>(count);
                    fill_array(arr.get(), count);
                    sattolo(arr.get(), count, rng);
                    warm_loop(arr.get(), count);

                    f64 best = std::numeric_limits<f64>::max();
                    for (i64 trial = 0; trial < CONTENTION_TRIALS;
                         ++trial) {
                        barrier.wait();
                        best = std::min(
                            best, timed_access(arr.get(),
                                               CONTENTION_ACCESSES));
                    }
                    per_thread[w] = best;
                });
            }
            for (std::thread& worker : workers) worker.join();

            f64 sum = 0.0;
            for (f64 ns : per_thread) sum += ns;
            measurements.push_back(Measurement{bytes, sum / t, t});
        }
    }
    show_progress(total_steps, total_steps, ENDING_SET_CONTENTION,
                  Axis::Bytes);
    std::fprintf(stderr, "\n");
    return SweepResult{Sweep::Thread, Axis::Bytes,
                       std::move(measurements)};
}

SweepResult cache_false_sharing() {
    constexpr i64 words_per_line = kcache_line_size / sizeof(u64);
    constexpr i32 writers = 2;
    std::vector<Line> lines((writers * END_SEPARATION) / kcache_line_size +
                            1);
    for (Line& line : lines)
        for (std::atomic<u64>& w : line.w) w.store(0);

    std::vector<Measurement> measurements;
    const i64 total_steps =
        doubling_steps(START_SEPARATION, END_SEPARATION);
    i64 step = 0;

    for (i64 sep = START_SEPARATION; sep <= END_SEPARATION; sep <<= 1) {
        show_progress(step++, total_steps, sep, Axis::Stride);

        SpinBarrier barrier(writers);
        f64 per_thread[writers] = {};
        std::vector<std::thread> workers;
        for (i32 t = 0; t < writers; ++t) {
            workers.emplace_back([&, t] {
                pin_to_p_cores();
                const i64 idx = (t * sep) / static_cast<i64>(sizeof(u64));
                std::atomic<u64>& slot =
                    lines[idx / words_per_line].w[idx % words_per_line];

                f64 best = std::numeric_limits<f64>::max();
                for (i64 trial = 0; trial < TRIALS; ++trial) {
                    barrier.wait();
                    const auto start = std::chrono::steady_clock::now();
                    for (i64 i = 0; i < FALSE_SHARING_WRITES; ++i)
                        slot.fetch_add(1, std::memory_order_relaxed);
                    const auto end = std::chrono::steady_clock::now();
                    best = std::min(best, elapsed_ns(start, end) /
                                              FALSE_SHARING_WRITES);
                }
                per_thread[t] = best;
            });
        }
        for (std::thread& worker : workers) worker.join();

        measurements.push_back(Measurement{
            sep, (per_thread[0] + per_thread[1]) / writers, writers});
    }
    show_progress(total_steps, total_steps, END_SEPARATION, Axis::Stride);
    std::fprintf(stderr, "\n");
    return SweepResult{Sweep::FalseSharing, Axis::Stride,
                       std::move(measurements)};
}
