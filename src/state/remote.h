#ifndef REMOTE_H
#define REMOTE_H

#include <stdint.h>
#include <structs/SettingPercents.h>
#include "machine.h"

extern SettingPercents settings;
extern sender s;

extern sml::sm<ossm_remote_state> *stateMachine;

// Initialize the state machine
void initStateMachine();

#endif // REMOTE_H