#ifndef LOCKBOX_LASTINTERACTION_H
#define LOCKBOX_LASTINTERACTION_H

#include <Arduino.h>

#ifdef SHORT_TIMEOUTS
static const unsigned long SLEEP_TIMEOUT = 30000;         // 30 seconds
static const unsigned long PSEUDO_SLEEP_TIMEOUT = 10000;  // 10 seconds
static const unsigned long IDLE_TIMEOUT = 5000;           // 5 seconds
#else
static const unsigned long SLEEP_TIMEOUT = 60000 * 10;        // 5 minutes
static const unsigned long PSEUDO_SLEEP_TIMEOUT = 60000 * 2;  // 2 minutes
static const unsigned long IDLE_TIMEOUT = 30000;              // 30 seconds
#endif

enum class IdleState { SLEEP, PSEUDO_SLEEP, IDLE, NOT_IDLE };

extern unsigned long sleepDuration;
static unsigned long lastInteraction = millis();
static IdleState idleState = IdleState::NOT_IDLE;

static void setNotIdle(String src) {
    lastInteraction = millis();

    if (idleState == IdleState::NOT_IDLE) {
        return;
    }
    idleState = IdleState::NOT_IDLE;
    ESP_LOGD("IDLE", "Setting idle state to NOT_IDLE, called from %s",
             src.c_str());
}

#endif  // LOCKBOX_LASTINTERACTION_H
