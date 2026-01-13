#include "crashpad_plus.h"

namespace crashpad_plus {

void
OnCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot)
{
    (void)process_snapshot;
}

void
OnNonCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot)
{
    (void)process_snapshot;
    // TODO: Implement later
}

} // namespace crashpad_plus
