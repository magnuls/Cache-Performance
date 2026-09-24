#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H
// Entry points for the sweeps. One declaration per experiment.

#include "measurement.h"
#include "system_info/system_info.h"
#include "types.h"

/*
 * Runs the full sweep from STARTING_SET to ENDING_SET.
 * Each Measurement is the minimum over repeated timed runs.
 */
Measurement measure_working_set(i64 bytes);
SweepResult cache_size_detection();
void refine_size_sweep(SweepResult& r);
SweepResult cache_line_size_detection(const AppleSystemInfo& s);
SweepResult cache_write_latency(const std::vector<Measurement>& read_measurements);
SweepResult cache_contention(const AppleSystemInfo& s);
SweepResult cache_false_sharing();

#endif
