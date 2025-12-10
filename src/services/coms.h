
#ifndef LOCKBOX_COMS_H
#define LOCKBOX_COMS_H

#include <ArduinoJson.h>
#include <NimBLEClient.h>
#include <NimBLEDevice.h>
#include <devices/device.h>
#include <devices/registry.hpp>
#include <structs/SettingPercents.h>

#include "state/remote.h"

struct DiscoveredDevice {
    const NimBLEAdvertisedDevice* advertisedDevice;
    const DeviceFactory* factory;
    std::string displayName;
    int rssi;
};

extern std::vector<DiscoveredDevice> discoveredDevices;

void clearDiscoveredDevices();

void sendCommand(const String& command);

void initBLE();

#endif