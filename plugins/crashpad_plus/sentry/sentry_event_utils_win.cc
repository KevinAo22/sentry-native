#include "sentry_event_utils.h"

#include "snapshot/win/process_snapshot_win.h"

using namespace crashpad;

namespace crashpad_plus {

void
EnrichSentryCrashEvent(const crashpad::ProcessSnapshot &process_snapshot)
{
    const auto &process_snapshot_win
        = static_cast<const ProcessSnapshotWin &>(process_snapshot);
    // TODO: Implement later
}

} // namespace crashpad_plus
