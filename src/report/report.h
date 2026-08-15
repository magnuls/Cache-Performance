#ifndef REPORT_H
#define REPORT_H
// Everything that turns Measurements into output: labels, progress, table, CSV.

#include <cstddef>

#include "measurement.h"
#include "system_info/system_info.h"
#include "types.h"

/*
 * Formats x for display. Axis decides the unit: Bytes and Stride are both byte
 * counts and share the B/K/M ladder, Threads is a bare count.
 */
void size_label(i64 x, Axis axis, char* out, size_t out_size);

/*
 * progress bar goes to stderr
 */
void show_progress(i64 step, i64 total, i64 x, Axis axis);

void display_measurements(const SweepResult& r);

/*
 * Path and x-column header both come from r, so no caller can name them wrong.
 */
void write_csv(const SweepResult& r, const SystemInfo& info);

#endif
