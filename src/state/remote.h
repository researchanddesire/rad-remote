#ifndef REMOTE_H
#define REMOTE_H

#include <stdint.h>
#include <structs/SettingPercents.h>
#include "machine.h"

extern SettingPercents settings;
extern sender s;
extern sml::sm<tcp_release> sm;

#endif // REMOTE_H