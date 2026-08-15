#include <pthread/qos.h>

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

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
int main() {
    // uses p-cores
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);

    const AppleSystemInfo info;
    assert(info.cache_line_size <= 128);
    info.print_summary();

    // Both are SweepResult; each one names its own csv file.
    auto size_detection = cache_size_detection();
    display_measurements(size_detection);
    write_csv(size_detection, info);

    std::cout << std::string(40, '-') << '\n';

    auto line_size_detection = cache_line_size_detection(info);
    display_measurements(line_size_detection);
    write_csv(line_size_detection, info);

    return 0;
}
