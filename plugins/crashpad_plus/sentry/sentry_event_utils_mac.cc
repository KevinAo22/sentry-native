#include "sentry_event_utils.h"

#include "snapshot/mac/process_snapshot_mac.h"

using namespace crashpad;

namespace crashpad_plus {

void
EnrichSentryCrashEvent(const crashpad::ProcessSnapshot &process_snapshot)
{
    const auto &process_snapshot_mac
        = static_cast<const ProcessSnapshotMac &>(process_snapshot);
    // TODO: Implement later
}

} // namespace crashpad_plus
