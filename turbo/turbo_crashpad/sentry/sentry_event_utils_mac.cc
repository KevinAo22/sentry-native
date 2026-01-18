#include "sentry_event_utils.h"

#include "snapshot/mac/process_snapshot_mac.h"

using namespace crashpad;

namespace turbo_crashpad {

void
EnrichSentryCrashEvent(const crashpad::ProcessSnapshot &process_snapshot)
{
    std::unordered_map<std::string, std::string> crash_details;

    const std::string uptime_str = GetProcessUptimeStr(process_snapshot);
    crash_details["uptime"] = uptime_str;

    const auto system_details = GetSystemDetails();
    const auto process_details = GetProcessDetails(process_snapshot);
}

std::unordered_map<std::string, std::string>
GetProcessDetails(const crashpad::ProcessSnapshot &process_snapshot)
{
    const auto &process_snapshot_mac
        = static_cast<const ProcessSnapshotMac &>(process_snapshot);
    std::unordered_map<std::string, std::string> process_details;

    return process_details;
}

std::unordered_map<std::string, std::string>
GetSystemDetails()
{
    std::unordered_map<std::string, std::string> system_details;

    return system_details;
}

} // namespace turbo_crashpad
