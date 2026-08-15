#ifndef MEASUREMENT_H
#define MEASUREMENT_H
// Shared vocabulary: what a sweep is, what its x axis means, and what it returns.

#include <vector>

#include "types.h"
/*
 * Sweep ->
 *
 */

enum class Sweep : u8 { Size, LineSize, Write, Thread };
enum class Axis : u8 { Bytes, Stride, Threads };
enum class Operation : u8 { Read, Write };

struct Measurement {
    i64 x;
    f64 ns_per_access;
};

struct SweepResult {
    Sweep kind;
    Axis axis;
    std::vector<Measurement> points;
};

#endif
