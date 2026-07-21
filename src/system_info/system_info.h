#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H
#include <cinttypes>
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
    const int core_count;
    const int64_t total_ram;
    const int64_t l1_cache;
    const int64_t l2_cache;

    virtual ~SystemInfo() = default;
    virtual void print_summary() const = 0;

    static const SystemInfo& get_instance();

    SystemInfo(const SystemInfo&) = delete;
    void operator=(const SystemInfo&) = delete;
    SystemInfo(const SystemInfo&&) = delete;
    void operator=(const SystemInfo&&) = delete;

   protected:
    SystemInfo(int cores, int64_t ram, int64_t l1, int64_t l2);
};

#if defined(__APPLE__)
// For Apple ARM Chips
struct AppleSystemInfo : SystemInfo {
    const int p_cores;
    const int e_cores;
    const int64_t slc_bytes;
    const int64_t p_l1i_cache;
    const int64_t p_l1d_cache;
    const int64_t e_l1i_cache;
    const int64_t e_l1d_cache;

    ~AppleSystemInfo() override = default;
    inline static const AppleSystemInfo& get_instance() {
        static AppleSystemInfo instance;
        return instance;
    }
    void print_summary() const override;

   private:
    AppleSystemInfo();
    static int64_t get(const char* name);
};
/*
 * Later on in the project I will implement system info for
 * AMD chips and Intel Chips (x86 architecture):w
 *

struct IntelSystemInfo : SystemInfo {
    const int p_cores;
    const int e_cores;
    const int64_t system_cache;
};

struct AMDSystemInfo : SystemInfo {};
*/
#endif
#endif
