#include "report/report.h"

#include <cstdio>
#include <vector>

namespace {

const char* csv_path(Sweep kind) {
    switch (kind) {
    case Sweep::Size:
        return "size_detection.csv";
    case Sweep::LineSize:
        return "cache_line_size_detection.csv";
    case Sweep::Write:
        return "write_detection.csv";
    case Sweep::Thread:
        return "contention.csv";
    case Sweep::FalseSharing:
        return "false_sharing.csv";
    }
    return "sweep.csv";
}

// Names the x column for whatever the sweep actually varied
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
        std::snprintf(out, out_size, "%lld",
                      static_cast<long long>(x));
        return;
    }

    f64 kib = x / 1024.0f;
    if (x < 1024) {
        std::snprintf(out, out_size, "%lldB",
                      static_cast<long long>(x));
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
    std::fprintf(stderr, "\r[%-*.*s] %2lld/%lld  %5s",
                 static_cast<i32>(kbar_width), filled,
                 "==============================",
                 static_cast<long long>(step),
                 static_cast<long long>(total), label);
    std::fflush(stderr);
}

/*
 * Aligned table to stdout for output in the terminal
 */
void display_measurements(const SweepResult& r) {
    std::printf("%10s, %5s, %18s, %7s\n", axis_header(r.axis), "label",
                "ns_per_access", "threads");
    for (const Measurement& m : r.points) {
        char label[8];
        size_label(m.x, r.axis, label, sizeof(label));
        std::printf("%10lld, %5s, %18.15g, %7d\n",
                    static_cast<long long>(m.x), label,
                    m.ns_per_access, m.threads);
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
    std::fprintf(f,
                 "%s,label,ns_per_access,threads,l1_bytes,l2_bytes,"
                 "l3_bytes,ram_bytes\n",
                 axis_header(r.axis));
    for (const Measurement& m : r.points) {
        char label[8];
        size_label(m.x, r.axis, label, sizeof(label));
        // Double is accurate to 15 significant digits, %g counts
        // those
        std::fprintf(f, "%lld,%s,%.15g,%d,%lld,%lld,%lld,%lld\n",
                     static_cast<long long>(m.x), label,
                     m.ns_per_access, m.threads,
                     static_cast<long long>(info.l1_cache),
                     static_cast<long long>(info.l2_cache),
                     static_cast<long long>(info.l3_cache),
                     static_cast<long long>(info.total_ram));
    }
    std::fclose(f);
}

namespace {

struct DetectionRow {
    const char* name;
    i64 detected;
    i64 next;
    i64 spec;
};

std::vector<DetectionRow> detection_rows(const Detected& d,
                                         const SystemInfo& info) {
    return {{"l1d", d.l1d, d.l1d_next, info.l1_cache},
            {"l2", d.l2, d.l2_next, info.l2_cache},
            {"line_size", d.line_size, -1, info.cache_line_size}};
}

void label_or_dash(i64 bytes, char* out, size_t out_size) {
    if (bytes >= 0)
        size_label(bytes, Axis::Bytes, out, out_size);
    else
        std::snprintf(out, out_size, "-");
}

} // namespace

void print_detection(const Detected& d, const SystemInfo& info) {
    std::printf("%-10s %10s %10s %10s %10s\n", "quantity", "plateau_end",
                "next_point", "sysctl", "spec/det");
    for (const DetectionRow& row : detection_rows(d, info)) {
        char detected[8], next[8], spec[8];
        label_or_dash(row.detected, detected, sizeof(detected));
        label_or_dash(row.next, next, sizeof(next));
        label_or_dash(row.spec, spec, sizeof(spec));
        if (row.detected > 0 && row.spec > 0)
            std::printf("%-10s %10s %10s %10s %9.2fx\n", row.name, detected,
                        next, spec,
                        static_cast<f64>(row.spec) / row.detected);
        else
            std::printf("%-10s %10s %10s %10s %10s\n", row.name, detected,
                        next, spec, "n/a");
    }
    std::fflush(stdout);
}

void write_detection_csv(const Detected& d, const SystemInfo& info) {
    const char* path = "detection.csv";
    FILE* f = std::fopen(path, "w");
    if (!f) {
        std::fprintf(stderr, "write_detection_csv: could not open %s\n",
                     path);
        return;
    }
    std::fprintf(f, "quantity,detected_bytes,next_bytes,spec_bytes\n");
    for (const DetectionRow& row : detection_rows(d, info))
        std::fprintf(f, "%s,%lld,%lld,%lld\n", row.name,
                     static_cast<long long>(row.detected),
                     static_cast<long long>(row.next),
                     static_cast<long long>(row.spec));
    std::fclose(f);
}
