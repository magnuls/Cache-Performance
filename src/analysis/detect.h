#ifndef DETECT_H
#define DETECT_H

#include <vector>

#include "cache_algorithms/config.h"
#include "measurement.h"
#include "types.h"

struct Cliff {
    i64 before;
    i64 after;
    f64 ratio;
};

struct Detected {
    i64 l1d = -1;
    i64 l1d_next = -1;
    i64 l2 = -1;
    i64 l2_next = -1;
    i64 line_size = -1;
};

std::vector<Cliff> find_cliffs(const std::vector<Measurement>& points,
                               f64 min_ratio = CLIFF_MIN_RATIO);

std::vector<i64> midpoints(i64 lo, i64 hi, i64 n, i64 granularity);

Detected detect_geometry(const SweepResult& size_sweep,
                         const SweepResult& false_sharing,
                         f64 min_ratio = CLIFF_MIN_RATIO);

#endif
