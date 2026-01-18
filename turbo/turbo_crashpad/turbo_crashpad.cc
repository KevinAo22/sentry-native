#include "turbo_crashpad.h"

#include "sentry/sentry_event_utils.h"

namespace turbo_crashpad {

void
OnCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot,
    const std::vector<base::FilePath> &attachments)
{
    EnrichSentryCrashEvent(process_snapshot, attachments);
}

void
OnNonCrashDumpEvent(const crashpad::ProcessSnapshot &process_snapshot)
{
    // TODO: Implement later
}

} // namespace turbo_crashpad
