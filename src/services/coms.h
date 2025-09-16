
#ifndef LOCKBOX_COMS_H
#define LOCKBOX_COMS_H

#include <NimBLEDevice.h>
#include <NimBLEClient.h>
#include <ArduinoJson.h>
#include <structs/SettingPercents.h>
#include "state/remote.h"
#include <devices/registry.hpp>
#include <devices/device.hpp>

// OSSM BLE Service and Characteristic UUIDs
#define OSSM_SERVICE_UUID "522B443A-4F53-534D-0001-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID "522B443A-4F53-534D-0002-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT "522B443A-4F53-534D-0010-420BADBABE69"
#define OSSM_DEVICE_NAME "OSSM"

extern Device *device;

void sendCommand(const String &command);

void initBLE();

#endif