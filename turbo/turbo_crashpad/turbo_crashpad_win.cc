#include "turbo_crashpad.h"

#include "sentry/sentry_event_utils.h"

namespace turbo_crashpad {

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

} // namespace turbo_crashpad
