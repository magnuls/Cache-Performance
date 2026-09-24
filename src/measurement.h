#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <vector>

#include "types.h"
/*
 * Sweep ->
 *
 */

enum class Sweep : u8 { Size, LineSize, Write, Thread, FalseSharing };
enum class Axis : u8 { Bytes, Stride, Threads };

struct Measurement {
    i64 x;
    f64 ns_per_access;
    i32 threads = 1;
};

struct SweepResult {
    Sweep kind;
    Axis axis;
    std::vector<Measurement> points;
};

#endif
