#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H
#include <vector>

#include "../types.h"

#ifndef __APPLE__
#error "cache_bench targets Apple Silicon"
#endif

struct Measurement {
    i64 buffer_bytes;
    f64 ns_per_access;
};

std::vector<Measurement> cache_size_detection(i64 N);
std::vector<Measurement> cache_line_size_detection(i64 N);

#endif
