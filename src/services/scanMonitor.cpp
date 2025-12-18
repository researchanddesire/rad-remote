#include "scanMonitor.h"
#include "state/remote.h"

void onScanComplete() {
    if (stateMachine) {
        stateMachine->process_event(devices_found_event());
    }
}
