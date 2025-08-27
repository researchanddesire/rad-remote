#ifndef REMOTE_H
#define REMOTE_H

#include <utils/RecusiveMutex.h>
#include <stdint.h>
#include <structs/SettingPercents.h>
#include "machine.h"
#include "utils/StateLogger.h"

extern SettingPercents settings;
// these are the last communicated settings to the OSSM.
extern SettingPercents lastSettings;

extern sender s;
extern ESP32RecursiveMutex mutex;
extern StateLogger stateLogger;

extern sml::sm<ossm_remote_state, sml::thread_safe<ESP32RecursiveMutex>, sml::logger<StateLogger>> *stateMachine;

// Initialize the state machine
void initStateMachine();

#endif // REMOTE_H