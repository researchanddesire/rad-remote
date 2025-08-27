
#ifndef LOCKBOX_COMS_H
#define LOCKBOX_COMS_H

#include <NimBLEDevice.h>
#include <NimBLEClient.h>
#include <ArduinoJson.h>
#include <structs/SettingPercents.h>
#include "state/remote.h"

// OSSM BLE Service and Characteristic UUIDs
#define OSSM_SERVICE_UUID "522B443A-4F53-534D-0001-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID "522B443A-4F53-534D-0002-420BADBABE69"
#define OSSM_DEVICE_NAME "OSSM"

void initBLE();

#endif