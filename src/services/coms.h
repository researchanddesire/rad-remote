
#ifndef LOCKBOX_COMS_H
#define LOCKBOX_COMS_H

#include <NimBLEDevice.h>
#include <NimBLEClient.h>
#include <ArduinoJson.h>
#include <structs/SettingPercents.h>
#include "state/remote.h"
#include <devices/registry.hpp>
#include <devices/device.h>

void sendCommand(const String &command);

void initBLE();

#endif