#ifndef CRASHPAD_PLUS_H
#define CRASHPAD_PLUS_H

#include "snapshot/process_snapshot.h"

namespace crashpad_plus {

void OnCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot);
void OnNonCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot);

} // namespace crashpad_plus

#endif // CRASHPAD_PLUS_H
