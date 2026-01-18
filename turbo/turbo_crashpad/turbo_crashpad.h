#ifndef TURBO_CRASHPAD_H
#define TURBO_CRASHPAD_H

#include "snapshot/process_snapshot.h"

#include <vector>

#include "base/files/file_path.h"

namespace turbo_crashpad {

void OnCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot,
    const std::vector<base::FilePath> &attachments);
void OnNonCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot);

} // namespace turbo_crashpad

#endif // TURBO_CRASHPAD_H
