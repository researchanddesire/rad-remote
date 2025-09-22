#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include <unordered_map>
#include <string>

#include <NimBLEUUID.h>
#include <Arduino.h>
#include "device.h"
#include "researchAndDesire/ossm/ossm_device.hpp"
#include "lovense/domi/domi_device.hpp"
#include "serviceUUIDs.h"

// Factory function type for creating device instances
typedef Device *(*DeviceFactory)(const NimBLEAdvertisedDevice *advertisedDevice);

// Map between uppercase service UUIDs and device factories
static const std::unordered_map<std::string, DeviceFactory> registry = {
    // clang-format off
    {OSSM_SERVICE_ID, [](const NimBLEAdvertisedDevice *advertisedDevice) -> Device * { return new OSSM(advertisedDevice); }},
    {DOMI_SERVICE_ID, [](const NimBLEAdvertisedDevice *advertisedDevice) -> Device * { return new Domi2(advertisedDevice); }}
    // clang-format on
};

inline const DeviceFactory *getDeviceFactory(const NimBLEUUID &serviceUUID)
{
    // Convert NimBLEUUID to Uppercase std::string for lookup
    std::string uuidStr = serviceUUID.toString().c_str();
    std::transform(uuidStr.begin(), uuidStr.end(), uuidStr.begin(), ::toupper);

    auto it = registry.find(uuidStr);

    if (it == registry.end())
        return nullptr;

    return &it->second;
}

#endif