#include "crashpad_plus.h"

#include "sentry/sentry_event_utils.h"

namespace crashpad_plus {

void
OnCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot)
{
    EnrichSentryCrashEvent(process_snapshot);
}

void
OnNonCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot)
{
    // TODO: Implement later
}

} // namespace crashpad_plus
