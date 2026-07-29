#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include "../types.h"
// EVERYTHING IS IN BYTES
/*
 * Cache architecture varies by vendor. L1/L2 sizes differ across chips,
 * and some (Apple Silicon) have no L3 at all, but instead a shared
 * system level cache.
 *
 * On Apple ARM chips, the base l1_cache and l2_cache fields hold
 * P-cluster values, and the L2 is SHARED by the whole P-cluster,
 * not private per core. On i32el/AMD, L2 is typically per-core.
 * So per-core math like l2_cache / core_count is not portable.
 *
 * For exact topology, use the derived struct for your chip.
 */

// Abstract Class
struct SystemInfo {
    const i32 core_count;
    const i64 total_ram;
    const i64 l1_cache;
    const i64 l2_cache;

    virtual ~SystemInfo() = default;
    virtual void print_summary() const = 0;

    static const SystemInfo& get_instance();

    SystemInfo(const SystemInfo&) = delete;
    void operator=(const SystemInfo&) = delete;
    SystemInfo(const SystemInfo&&) = delete;
    void operator=(const SystemInfo&&) = delete;

   protected:
    SystemInfo(i32 cores, i64 ram, i64 l1, i64 l2);
};

#if defined(__APPLE__)
// For Apple ARM Chips
struct AppleSystemInfo : SystemInfo {
    const i32 p_cores;
    const i32 e_cores;
    const i64 slc_bytes;
    const i64 p_l1i_cache;
    const i64 p_l1d_cache;
    const i64 e_l1i_cache;
    const i64 e_l1d_cache;

    ~AppleSystemInfo() override = default;
    inline static const AppleSystemInfo& get_instance() {
        static AppleSystemInfo instance;
        return instance;
    }
    void print_summary() const override;

   private:
    AppleSystemInfo();
    static i64 get(const char* name);
};
/*
 * Later on in the project I will implement system info for
 * AMD chips and i32el Chips (x86 architecture):w
 *

struct i32elSystemInfo : SystemInfo {
    const i32 p_cores;
    const i32 e_cores;
    const i64 system_cache;
};

struct AMDSystemInfo : SystemInfo {};
*/
#endif
#endif
