#ifndef SENTRY_EVENT_UTILS_H
#define SENTRY_EVENT_UTILS_H

#include "snapshot/process_snapshot.h"

namespace turbo_crashpad {

void EnrichSentryCrashEvent(const crashpad::ProcessSnapshot &process_snapshot);

} // namespace turbo_crashpad

#endif // SENTRY_EVENT_UTILS_H
