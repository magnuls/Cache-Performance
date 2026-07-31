#include "system_info.h"

#include "../types.h"

SystemInfo::SystemInfo(i32 cores, i64 ram, i64 l1, i64 l2, i64 line_size,
                       i64 page_size)
    : core_count(cores),
      total_ram(ram),
      l1_cache(l1),
      l2_cache(l2),
      cache_line_size(line_size),
      page_size(page_size) {};

#if defined(__APPLE__)
#include <sys/sysctl.h>

#include <cstdio>
#include <cstring>

i64 AppleSystemInfo::get(const char* name) {
    i64 value = 0;
    size_t size = sizeof(value);
    sysctlbyname(name, &value, &size, nullptr, 0);
    return value;
}

std::string AppleSystemInfo::get_string(const char* name) {
    char buffer[128] = {};
    size_t size = sizeof(buffer) - 1;
    if (sysctlbyname(name, buffer, &size, nullptr, 0) != 0) return "unknown";
    return buffer;
}

// RAM type/manufacturer aren't in sysctl so I parsed them out of
// `system_profiler SPMemoryDataType` ->
//       Memory: 36 GB
//       Type: LPDDR5
//       Manufacturer: Micron
const AppleSystemInfo::RamInfo& AppleSystemInfo::get_ram_info() {
    static const RamInfo cached = [] {
        RamInfo info{"unknown", "unknown"};
        FILE* pipe = popen("system_profiler SPMemoryDataType 2>/dev/null", "r");
        if (!pipe) return info;

        auto value_after = [](const std::string& line, const char* key) {
            size_t pos = line.find(key);
            if (pos == std::string::npos) return std::string();
            std::string v = line.substr(pos + std::strlen(key));
            v.erase(0, v.find_first_not_of(" \t"));
            v.erase(v.find_last_not_of(" \t\n\r") + 1);
            return v;
        };

        char line[256];
        while (std::fgets(line, sizeof(line), pipe)) {
            std::string v = value_after(line, "Type:");
            if (!v.empty()) info.type = v;
            v = value_after(line, "Manufacturer:");
            if (!v.empty()) info.manufacturer = v;
        }
        pclose(pipe);
        return info;
    }();
    return cached;
}
// hw.perflevel0.l1dcachesize
AppleSystemInfo::AppleSystemInfo()
    : SystemInfo(get("hw.physicalcpu"), get("hw.memsize"),
                 get("hw.perflevel0.l1dcachesize"),

                 get("hw.perflevel0.l2cachesize"), get("hw.cachelinesize"),
                 get("hw.pagesize")),
      p_cores(get("hw.perflevel0.physicalcpu")),
      e_cores(get("hw.perflevel1.physicalcpu")),
      slc_bytes(-1),  // not exposed by sysctl
      p_l1i_cache(get("hw.perflevel0.l1icachesize")),
      p_l1d_cache(get("hw.perflevel0.l1dcachesize")),
      e_l1i_cache(get("hw.perflevel1.l1icachesize")),
      e_l1d_cache(get("hw.perflevel1.l1dcachesize")),
      e_l2_cache(get("hw.perflevel1.l2cachesize")),
      p_cpus_per_l2(get("hw.perflevel0.cpusperl2")),
      e_cpus_per_l2(get("hw.perflevel1.cpusperl2")),
      p_clusters(p_cpus_per_l2 > 0 ? p_cores / p_cpus_per_l2 : 0),
      e_clusters(e_cpus_per_l2 > 0 ? e_cores / e_cpus_per_l2 : 0),
      chip_name(get_string("machdep.cpu.brand_string")),
      ram_type(get_ram_info().type),
      ram_manufacturer(get_ram_info().manufacturer) {}

void AppleSystemInfo::print_summary() const {
    auto mib = [](i64 bytes) { return bytes / (1024.0 * 1024.0); };
    auto kib = [](i64 bytes) { return bytes / 1024.0; };

    std::printf("Apple Silicon System Info\n");
    std::printf("-------------------------\n");
    std::printf("Chip:        %s\n", chip_name.c_str());
    std::printf("Cores:       %d total (%d P + %d E)\n", core_count, p_cores,
                e_cores);
    std::printf(
        "Clusters:    %d P-cluster(s) of %d cores, %d E-cluster(s) of "
        "%d cores\n",
        p_clusters, p_cpus_per_l2, e_clusters, e_cpus_per_l2);
    std::printf("RAM:         %.1f GiB %s (%s)\n",
                total_ram / (1024.0 * 1024.0 * 1024.0), ram_type.c_str(),
                ram_manufacturer.c_str());
    std::printf("P-core L1:   %.0f KiB inst / %.0f KiB data\n",
                kib(p_l1i_cache), kib(p_l1d_cache));
    std::printf("E-core L1:   %.0f KiB inst / %.0f KiB data\n",
                kib(e_l1i_cache), kib(e_l1d_cache));
    std::printf("P-cluster L2: %.1f MiB (shared by %d cores)\n", mib(l2_cache),
                p_cpus_per_l2);
    std::printf("E-cluster L2: %.1f MiB (shared by %d cores)\n",
                mib(e_l2_cache), e_cpus_per_l2);
    if (slc_bytes >= 0)
        std::printf("SLC:         %.1f MiB\n", mib(slc_bytes));
    else
        std::printf("SLC:         unavailable\n");
    std::printf("Cache line:  %lld B\n", (long long)cache_line_size);
    std::printf("Page size:   %.0f KiB\n", kib(page_size));
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
