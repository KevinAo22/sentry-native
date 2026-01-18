#ifndef SENTRY_EVENT_UTILS_H
#define SENTRY_EVENT_UTILS_H

#include <unordered_map>
#include <vector>

#include "base/files/file_path.h"
#include "snapshot/process_snapshot.h"

namespace turbo_crashpad {

void EnrichSentryCrashEvent(const crashpad::ProcessSnapshot &process_snapshot,
    const std::vector<base::FilePath> &attachments);

std::string GetProcessUptimeStr(
    const crashpad::ProcessSnapshot &process_snapshot);

std::unordered_map<std::string, std::string> GetProcessDetails(
    const crashpad::ProcessSnapshot &process_snapshot);

std::unordered_map<std::string, std::string> GetSystemDetails();

struct ModuleInfo {
    std::string name;
    std::string full_path;
    std::string category = "unknown";
};

ModuleInfo GetExceptionModuleInfo(
    const crashpad::ProcessSnapshot &process_snapshot);

} // namespace turbo_crashpad

#endif // SENTRY_EVENT_UTILS_H
