#include <pthread/qos.h>

#include <cassert>
#include <iostream>

#include "cache_algorithms/experiments.h"
#include "system_info/system_info.h"

using namespace std;
int main() {
    // Uses P-Cores
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    /*
    const AppleSystemInfo& info = AppleSystemInfo::get_instance();
    */
    assert(info.cache_line_size <= 128);
    std::vector v = cache_size_detection();
    display_measurements(v);
    // Relative to the directory you launch from (pls run from the Repo Root)
    // Need plot.py to find the csv
    write_csv(v, "results.csv");

    return 0;
}
