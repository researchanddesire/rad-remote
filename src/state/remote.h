#ifndef REMOTE_H
#define REMOTE_H

#include <stdint.h>
#include <structs/SettingPercents.h>
#include "machine.h"

extern SettingPercents settings;
extern sender s;

// Initialize the state machine
void initStateMachine();

#endif // REMOTE_H