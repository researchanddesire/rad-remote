#include "lastInteraction.h"

// Actual definitions of the shared variables
// volatile ensures compiler doesn't cache these across threads
volatile unsigned long sleepDuration = 0;
volatile unsigned long lastInteraction = 0;  // Will be set properly in setupIdleMonitor()
volatile IdleState idleState = IdleState::NOT_IDLE;
