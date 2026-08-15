#include "report/report.h"

#include <cstdio>

namespace {

/*
 * One output file per experiment. Deriving this from the result is what stops
 * two sweeps writing over each other.
 */
const char* csv_path(Sweep kind) {
    switch (kind) {
    case Sweep::Size:
        return "size_detection.csv";
    case Sweep::LineSize:
        return "cache_line_size_detection.csv";
    case Sweep::Write:
        return "write_detection.csv";
    case Sweep::Thread:
        return "thread_detection.csv";
    }
    return "sweep.csv";
}

// Names the x column for whatever the sweep actually varied.
const char* axis_header(Axis axis) {
    switch (axis) {
    case Axis::Bytes:
        return "size_bytes";
    case Axis::Stride:
        return "stride_bytes";
    case Axis::Threads:
        return "threads";
    }
    return "x";
}

} // namespace

void size_label(i64 x, Axis axis, char* out, size_t out_size) {
    if (axis == Axis::Threads) {
        std::snprintf(out, out_size, "%lld", static_cast<long long>(x));
        return;
    }

    f64 kib = x / 1024.0f;
    if (x < 1024) {
        std::snprintf(out, out_size, "%lldB", static_cast<long long>(x));
    } else if (kib < 1024.0f) {
        // Rounds numbers btw
        std::snprintf(out, out_size, "%.0fK", kib);
    } else {
        std::snprintf(out, out_size, "%.0fM", kib / 1024.0f);
    }
}

/*
 * progress bar goes to stderr
 */
void show_progress(i64 step, i64 total, i64 x, Axis axis) {
    constexpr i64 kbar_width = 30;
    i32 filled = static_cast<i32>(step * kbar_width / total);
    char label[8];
    size_label(x, axis, label, sizeof(label));
    std::fprintf(stderr, "\r[%-*.*s] %2lld/%lld  %5s", static_cast<i32>(kbar_width), filled,
                 "==============================", static_cast<long long>(step),
                 static_cast<long long>(total), label);
    std::fflush(stderr);
}

/*
 * Aligned table to stdout for output in the terminal
 */
void display_measurements(const SweepResult& r) {
    std::printf("%10s, %5s, %18s\n", axis_header(r.axis), "label", "ns_per_access");
    for (const Measurement& m : r.points) {
        char label[8];
        size_label(m.x, r.axis, label, sizeof(label));
        std::printf("%10lld, %5s, %18.15g\n", static_cast<long long>(m.x), label, m.ns_per_access);
    }

    std::fflush(stdout); // flush console output
}

/*
 *
 * Writes to a csv file.
 */
void write_csv(const SweepResult& r, const SystemInfo& info) {
    const char* path = csv_path(r.kind);
    FILE* f = std::fopen(path, "w");
    if (!f) {
        std::fprintf(stderr, "write_csv: could not open %s\n", path);
        return;
    }
    std::fprintf(f, "%s,label,ns_per_access,l1_bytes,l2_bytes,l3_bytes,ram_bytes\n",
                 axis_header(r.axis));
    for (const Measurement& m : r.points) {
        char label[8];
        size_label(m.x, r.axis, label, sizeof(label));
        // Double is accurate to 15 significant digits, %g counts those
        std::fprintf(f, "%lld,%s,%.15g,%lld,%lld,%lld,%lld\n", static_cast<long long>(m.x), label,
                     m.ns_per_access, static_cast<long long>(info.l1_cache),
                     static_cast<long long>(info.l2_cache), static_cast<long long>(info.l3_cache),
                     static_cast<long long>(info.total_ram));
    }
    std::fclose(f);
}
