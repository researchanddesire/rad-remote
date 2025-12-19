
#ifndef LOCKBOX_COMS_H
#define LOCKBOX_COMS_H

#include <NimBLEDevice.h>
#include <NimBLEClient.h>
#include <ArduinoJson.h>
#include <structs/SettingPercents.h>
#include "state/remote.h"
#include <devices/registry.hpp>
#include <devices/device.h>
#include <vector>

struct DiscoveredDevice {
    const NimBLEAdvertisedDevice *advertisedDevice;
    const DeviceFactory *factory;
    std::string name;
    int rssi;
};

void sendCommand(const String &command);

void initBLE();

// Device list management
std::vector<DiscoveredDevice>& getDiscoveredDevices();
void clearDiscoveredDevices();
void connectToDiscoveredDevice(int index);
void startScanWithTimeout(int timeoutMs, void (*onComplete)());

#endif