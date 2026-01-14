#ifndef SENTRY_EVENT_UTILS_H
#define SENTRY_EVENT_UTILS_H

#include "snapshot/process_snapshot.h"

namespace crashpad_plus {

void EnrichSentryCrashEvent(const crashpad::ProcessSnapshot &process_snapshot);

} // namespace crashpad_plus

#endif // SENTRY_EVENT_UTILS_H
