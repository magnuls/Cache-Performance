#include "system_info.h"

SystemInfo::SystemInfo(int cores, int64_t ram, int64_t l1, int64_t l2)
    : core_count(cores), total_ram(ram), l1_cache(l1), l2_cache(l2) {};

#if defined(__APPLE__)
#include <sys/sysctl.h>

#include <cstdio>

int64_t AppleSystemInfo::get(const char* name) {
    int64_t value = 0;
    size_t size = sizeof(value);
    sysctlbyname(name, &value, &size, nullptr, 0);
    return value;
}
// hw.perflevel0.l1dcachesize
AppleSystemInfo::AppleSystemInfo()
    : SystemInfo(get("hw.physicalcpu"),              // total cores
                 get("hw.memsize"),                  // ram_bytes
                 get("hw.perflevel0.l1dcachesize"),  // base l1 (P d-cache,
                                                     // documented choice)
                 get("hw.perflevel0.l2cachesize")),  // base l2 (P-cluster L2)
      p_cores(get("hw.perflevel0.physicalcpu")),
      e_cores(get("hw.perflevel1.physicalcpu")),
      slc_bytes(-1),  // not exposed by sysctl
      p_l1i_cache(get("hw.perflevel0.l1icachesize")),
      p_l1d_cache(get("hw.perflevel0.l1dcachesize")),
      e_l1i_cache(get("hw.perflevel1.l1icachesize")),
      e_l1d_cache(get("hw.perflevel1.l1dcachesize")) {}

void AppleSystemInfo::print_summary() const {
    auto mib = [](int64_t bytes) { return bytes / (1024.0 * 1024.0); };
    auto kib = [](int64_t bytes) { return bytes / 1024.0; };

    std::printf("Apple Silicon System Info\n");
    std::printf("-------------------------\n");
    std::printf("Cores:       %d total (%d P + %d E)\n", core_count, p_cores,
                e_cores);
    std::printf("RAM:         %.1f GiB\n",
                total_ram / (1024.0 * 1024.0 * 1024.0));
    std::printf("P-core L1:   %.0f KiB inst / %.0f KiB data\n",
                kib(p_l1i_cache), kib(p_l1d_cache));
    std::printf("E-core L1:   %.0f KiB inst / %.0f KiB data\n",
                kib(e_l1i_cache), kib(e_l1d_cache));
    std::printf("L2:          %.1f MiB\n", mib(l2_cache));
    if (slc_bytes >= 0)
        std::printf("SLC:         %.1f MiB\n", mib(slc_bytes));
    else
        std::printf("SLC:         unavailable\n");
}

#endif
/*
 * # Everything cache-related at once
sysctl -a | grep -i cache

# Cache line size (returns 128 on Apple silicon)
sysctl hw.cachelinesize

# Per-core-type caches: perflevel0 = P-cores, perflevel1 = E-cores
sysctl hw.perflevel0.l1icachesize   # P-core L1 instruction
sysctl hw.perflevel0.l1dcachesize   # P-core L1 data
sysctl hw.perflevel0.l2cachesize    # P-cluster shared L2
sysctl hw.perflevel0.cpusperl2      # how many cores share that L2 (the cluster
size!)

sysctl hw.perflevel1.l1icachesize   # E-core L1i
sysctl hw.perflevel1.l1dcachesize   # E-core L1d
sysctl hw.perflevel1.l2cachesize    # E-cluster shared L2
sysctl hw.perflevel1.cpusperl2

# Core counts per type
sysctl hw.perflevel0.physicalcpu hw.perflevel1.physicalcpu
*/
