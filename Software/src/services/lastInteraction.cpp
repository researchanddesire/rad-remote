#include "lastInteraction.h"

// Actual definitions of the shared variables
unsigned long sleepDuration = 0;
unsigned long lastInteraction = 0;  // Will be set properly in setupIdleMonitor()
IdleState idleState = IdleState::NOT_IDLE;
