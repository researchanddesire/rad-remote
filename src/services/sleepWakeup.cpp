#include "sleepWakeup.h"

#include "state/remote.h"

void sendWakeUpEvent() {
    if (stateMachine != nullptr) {
        stateMachine->process_event(wake_up_event());
    }
}