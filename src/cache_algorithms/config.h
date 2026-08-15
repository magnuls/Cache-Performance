#ifndef CONFIG_H
#define CONFIG_H
// Tuning knobs for every sweep: axis bounds and how many trials to keep the min of.

#include "types.h"

/*
 * Sweep bounds. Our starting set  for readingis 4KB, our ending set is 256MB.
 * 2^12 = 4096 bytes, 2^28 = 268,435,456 bytes.
 */
inline constexpr i64 STARTING_SET_READ = i64{1} << 12;
inline constexpr i64 ENDING_SET_READ = i64{1} << 28;
inline constexpr i64 TRIALS = 5;

// 8 bytes -> 1024
inline constexpr i64 STARTING_SET_WRITE = i64{1} << 3;
inline constexpr i64 ENDING_SET_WRITE = i64{1} << 10;

// Stride Lengths (4B -> 512B)
inline constexpr i64 START_STRIDE_LENGTH = i64{1} << 2;
inline constexpr i64 END_STRIDE_LENGTH = i64{1} << 9;

#endif
