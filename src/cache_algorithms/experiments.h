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
SweepResult cache_size_detection();
SweepResult cache_line_size_detection(const AppleSystemInfo& s);

#endif
