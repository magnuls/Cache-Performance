#ifndef CONFIG_H
#define CONFIG_H
// Tuning knobs for every sweep: axis bounds and how many trials to
// keep the min of.

#include "types.h"

/*
 * Sweep bounds. Our starting set  for readingis 4KB, our ending set
 * is 256MB. 2^12 = 4096 bytes, 2^28 = 268,435,456 bytes.
 */
inline constexpr i64 STARTING_SET_READ = i64{1} << 12;
inline constexpr i64 ENDING_SET_READ = i64{1} << 28;
inline constexpr i64 TRIALS = 5;

// 8 bytes -> 1024
inline constexpr i64 STARTING_SET_WRITE = i64{1} << 12;
inline constexpr i64 ENDING_SET_WRITE = i64{1} << 28;

// Stride Lengths (4B -> 2KB)
inline constexpr i64 START_STRIDE_LENGTH = i64{1} << 2;
inline constexpr i64 END_STRIDE_LENGTH = i64{1} << 11;

inline constexpr i64 STARTING_SET_CONTENTION = i64{1} << 16;
inline constexpr i64 ENDING_SET_CONTENTION = i64{1} << 26;
inline constexpr i64 CONTENTION_ACCESSES = 10000000;
inline constexpr i64 CONTENTION_TRIALS = 3;

inline constexpr i64 START_SEPARATION = 8;
inline constexpr i64 END_SEPARATION = 1024;
inline constexpr i64 FALSE_SHARING_WRITES = 5000000;

inline constexpr i64 REFINE_POINTS = 3;
inline constexpr f64 CLIFF_MIN_RATIO = 1.4;

#endif
