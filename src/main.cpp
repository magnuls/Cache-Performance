#include <pthread/qos.h>

#include <cassert>
#include <cstddef>
#include <iostream>
#include <set>
#include <string>

#include "analysis/detect.h"
#include "cache_algorithms/experiments.h"
#include "report/report.h"
#include "system_info/system_info.h"

/*
 * Please run the source code from the project root directory
 *
 * 'write_csv()' will output csv files in the project directory
 * in which the source code is run from!
 */

using namespace std;

namespace {

const std::set<std::string> kSweeps{"size", "line", "write", "contention",
                                    "false_sharing"};

void separator() { std::cout << std::string(40, '-') << '\n'; }

} // namespace

int main(int argc, char** argv) {
    std::set<std::string> want(argv + 1, argv + argc);
    for (const std::string& w : want) {
        if (!kSweeps.count(w)) {
            std::cerr << "usage: cache_bench [size] [line] [write] "
                         "[contention] [false_sharing]\n";
            return 1;
        }
    }
    auto run = [&](const char* name) {
        return want.empty() || want.count(name) > 0;
    };

    // uses p-cores
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);

    const AppleSystemInfo info;
    assert(info.cache_line_size <= 128);
    info.print_summary();

    SweepResult size_detection{Sweep::Size, Axis::Bytes, {}};
    SweepResult false_sharing{Sweep::FalseSharing, Axis::Stride, {}};

    if (run("size") || run("write")) {
        size_detection = cache_size_detection();
        refine_size_sweep(size_detection);
        display_measurements(size_detection);
        write_csv(size_detection, info);
        separator();
    }

    if (run("line")) {
        auto line_size_detection = cache_line_size_detection(info);
        display_measurements(line_size_detection);
        write_csv(line_size_detection, info);
        separator();
    }

    if (run("write")) {
        auto write_measurement =
            cache_write_latency(size_detection.points);
        display_measurements(write_measurement);
        write_csv(write_measurement, info);
        separator();
    }

    if (run("contention")) {
        auto contention = cache_contention(info);
        display_measurements(contention);
        write_csv(contention, info);
        separator();
    }

    if (run("false_sharing")) {
        false_sharing = cache_false_sharing();
        display_measurements(false_sharing);
        write_csv(false_sharing, info);
        separator();
    }

    if (!size_detection.points.empty() || !false_sharing.points.empty()) {
        const Detected detected =
            detect_geometry(size_detection, false_sharing);
        print_detection(detected, info);
        write_detection_csv(detected, info);
    }
    return 0;
}
