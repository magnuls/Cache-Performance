#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <string>

#include "../types.h"
// EVERYTHING IS IN BYTES
/*
 * Cache architecture varies by vendor. L1/L2 sizes differ across chips,
 * and some (Apple Silicon) have no L3 at all, but instead a shared
 * system level cache.
 *
 * On Apple ARM chips, the base l1_cache and l2_cache fields hold
 * P-cluster values, and the L2 is SHARED by the whole P-cluster,
 * not private per core. On Intel/AMD, L2 is typically per-core.
 * So per-core math like l2_cache / core_count is not portable.
 *
 * For exact topology, use the derived struct for your chip.
 */

// Abstract Class
struct SystemInfo {
    i32 core_count;
    i64 total_ram;
    i64 l1_cache;
    i64 l2_cache;
    // On Apple this is the system level cache (SLC), macOS does not
    // expose its size so it stays -1
    i64 l3_cache;
    i64 cache_line_size;
    // First untimed warm-up pass exists to pay these faults up front.
    i64 page_size;

    virtual ~SystemInfo() = default;
    virtual void print_summary() const = 0;

    SystemInfo(const SystemInfo&) = default;
    SystemInfo& operator=(const SystemInfo&) = default;
    SystemInfo(SystemInfo&&) = default;
    SystemInfo& operator=(SystemInfo&&) = default;

   protected:
    SystemInfo(i32 cores, i64 ram, i64 l1, i64 l2, i64 l3, i64 line_size,
               i64 page_size);
};

#if defined(__APPLE__)
// For Apple ARM Chips
struct AppleSystemInfo : SystemInfo {
    i32 p_cores;
    i32 e_cores;
    i64 slc_bytes;
    i64 p_l1i_cache;
    i64 p_l1d_cache;
    i64 e_l1i_cache;
    i64 e_l1d_cache;
    // E-cluster shared L2 (base l2_cache holds the P-cluster value).
    i64 e_l2_cache;
    // How many cores share each L2  needed to interpret the multi
    // threaded sweep where cluster buddies contend for the same L2.
    i32 p_cpus_per_l2;
    i32 e_cpus_per_l2;
    // Number of clusters of each type (cores / cores-per-cluster).
    i32 p_clusters;
    i32 e_clusters;
    std::string chip_name;
    // Not exposed by sysctl, so these come
    // from system_profiler SPMemoryDataType
    std::string ram_type;
    std::string ram_manufacturer;

    AppleSystemInfo();
    void print_summary() const override;

   private:
    static i64 get(const char* name);
    static std::string get_string(const char* name);

    struct RamInfo {
        std::string type;
        std::string manufacturer;
    };
    static const RamInfo& get_ram_info();
};
/*
 * Later on in the project I will implement system info for
 * AMD chips and Intel chips (x86 architecture)
 *

struct IntelSystemInfo : SystemInfo {
    const i32 p_cores;
    const i32 e_cores;
    const i64 system_cache;
};

struct AMDSystemInfo : SystemInfo {};
*/
#endif
#endif
