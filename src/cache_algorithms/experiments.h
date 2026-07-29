#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H
#include <chrono>
#include <limits>

#include "../system_info/system_info.h"
#include "../types.h"

#if defined(__APPLE__)
const AppleSystemInfo& info = AppleSystemInfo::get_instance();

void size_detection(i64 N) {
    // Solution
}

#endif
#endif
