#include "sentry_event_utils.h"

#include <Psapi.h>

#include <cstdint>
#include <unordered_map>

#include "snapshot/win/process_snapshot_win.h"

using namespace crashpad;

namespace turbo_crashpad {

std::unordered_map<std::string, std::string>
GetProcessDetails(const crashpad::ProcessSnapshot &process_snapshot)
{
    const auto &process_snapshot_win
        = static_cast<const ProcessSnapshotWin &>(process_snapshot);
    std::unordered_map<std::string, std::string> process_details;

    process_details["uptime"] = GetProcessUptimeStr(process_snapshot);
    process_details["thread_count"]
        = std::to_string(process_snapshot_win.Threads().size());
    process_details["handle_count"]
        = std::to_string(process_snapshot_win.Handles().size());

    // CPU usage
    timeval user_time, system_time;
    process_snapshot.ProcessCPUTimes(&user_time, &system_time);
    process_details["cpu_user_time_ms"]
        = std::to_string(user_time.tv_sec * 1000 + user_time.tv_usec / 1000);
    process_details["cpu_kernel_time_ms"] = std::to_string(
        system_time.tv_sec * 1000 + system_time.tv_usec / 1000);

    // Memory usage
    uint64_t max_free_region_size = 0, total_free_region_size = 0;
    uint64_t max_commit_region_size = 0, total_commit_region_size = 0;
    uint64_t max_reserve_region_size = 0, total_reserve_region_size = 0;
    uint64_t total_commit_image_region_size = 0;
    uint64_t total_commit_mapped_region_size = 0;
    uint64_t total_commit_private_region_size = 0;

    const auto &regions = process_snapshot_win.MemoryMap();
    for (const auto *e : regions) {
        auto &region = e->AsMinidumpMemoryInfo();
        if (region.State == MEM_FREE) {
            total_free_region_size += region.RegionSize;
            if (region.RegionSize > max_free_region_size) {
                max_free_region_size = region.RegionSize;
            }
        } else if (region.State == MEM_COMMIT) {
            total_commit_region_size += region.RegionSize;
            if (region.RegionSize > max_commit_region_size) {
                max_commit_region_size = region.RegionSize;
            }

            switch (region.Type) {
            case MEM_IMAGE:
                total_commit_image_region_size += region.RegionSize;
                break;
            case MEM_MAPPED:
                total_commit_mapped_region_size += region.RegionSize;
                break;
            case MEM_PRIVATE:
                total_commit_private_region_size += region.RegionSize;
                break;
            default:
                break;
            }
        } else if (region.State == MEM_RESERVE) {
            total_reserve_region_size += region.RegionSize;
            if (region.RegionSize > max_reserve_region_size) {
                max_reserve_region_size = region.RegionSize;
            }
        }
    }

    process_details["max_free_region_size"]
        = std::to_string(max_free_region_size);
    process_details["total_free_region_size"]
        = std::to_string(total_free_region_size);
    process_details["max_commit_region_size"]
        = std::to_string(max_commit_region_size);
    process_details["total_commit_region_size"]
        = std::to_string(total_commit_region_size);
    process_details["max_reserve_region_size"]
        = std::to_string(max_reserve_region_size);
    process_details["total_reserve_region_size"]
        = std::to_string(total_reserve_region_size);
    process_details["total_commit_image_region_size"]
        = std::to_string(total_commit_image_region_size);
    process_details["total_commit_mapped_region_size"]
        = std::to_string(total_commit_mapped_region_size);
    process_details["total_commit_private_region_size"]
        = std::to_string(total_commit_private_region_size);

    HANDLE process = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE, process_snapshot.ProcessID());
    if (process) {
        PROCESS_MEMORY_COUNTERS_EX pmc = { sizeof(pmc) };
        if (::GetProcessMemoryInfo(
                process, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc))) {
            process_details["working_set_size"]
                = std::to_string(pmc.WorkingSetSize);
            process_details["peak_working_set_size"]
                = std::to_string(pmc.PeakWorkingSetSize);
        }

        ::CloseHandle(process);
    }

    return process_details;
}

std::unordered_map<std::string, std::string>
GetSystemDetails()
{
    std::unordered_map<std::string, std::string> system_details;

    PERFORMANCE_INFORMATION perf_info = { sizeof(perf_info) };
    if (::GetPerformanceInfo(&perf_info, sizeof(perf_info))) {
        uint64_t commit_total_size = perf_info.CommitTotal;
        uint64_t commit_limit_size = perf_info.CommitLimit;
        double commit_percent = static_cast<double>(commit_total_size)
            / commit_limit_size * 100.0;
        uint64_t physical_total_size = perf_info.PhysicalTotal;
        uint64_t physical_available_size = perf_info.PhysicalAvailable;
        double physical_percent = static_cast<double>(physical_available_size)
            / physical_total_size * 100.0;
        uint64_t kernel_total_size = perf_info.KernelTotal;
        uint64_t kernel_paged_size = perf_info.KernelPaged;
        uint64_t kernel_nonpaged_size = perf_info.KernelNonpaged;
        uint64_t cache_size = perf_info.SystemCache;
        uint64_t handle_count = perf_info.HandleCount;
        uint64_t process_count = perf_info.ProcessCount;
        uint64_t thread_count = perf_info.ThreadCount;

        char buffer[32];
        system_details["commit_total_size"] = std::to_string(commit_total_size);
        system_details["commit_limit_size"] = std::to_string(commit_limit_size);
        std::snprintf(buffer, sizeof(buffer), "%.2f%%", commit_percent);
        system_details["commit_percent"] = std::string(buffer) + "%";

        system_details["physical_total_size"]
            = std::to_string(physical_total_size);
        system_details["physical_available_size"]
            = std::to_string(physical_available_size);
        std::snprintf(buffer, sizeof(buffer), "%.2f%%", physical_percent);
        system_details["physical_usage"] = std::string(buffer) + "%";

        system_details["kernel_total_size"] = std::to_string(kernel_total_size);
        system_details["kernel_paged_size"] = std::to_string(kernel_paged_size);
        system_details["kernel_nonpaged_size"]
            = std::to_string(kernel_nonpaged_size);
        system_details["cache_size"] = std::to_string(cache_size);
        system_details["handle_count"] = std::to_string(handle_count);
        system_details["process_count"] = std::to_string(process_count);
        system_details["thread_count"] = std::to_string(thread_count);
    }

    return system_details;
}

} // namespace turbo_crashpad
