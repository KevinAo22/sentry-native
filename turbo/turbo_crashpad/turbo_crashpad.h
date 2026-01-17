#ifndef TURBO_CRASHPAD_H
#define TURBO_CRASHPAD_H

#include "snapshot/process_snapshot.h"

namespace turbo_crashpad {

void OnCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot);
void OnNonCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot);

} // namespace turbo_crashpad

#endif // TURBO_CRASHPAD_H
