#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include <unordered_map>
#include <string>

#include <Arduino.h>
#include "device.hpp"
#include "researchAndDesire/ossm/ossm_device.h"
#include "lovense/domi/domi_device.h"
#include "serviceUUIDs.h"

// Factory function type for creating device instances
typedef Device *(*DeviceFactory)();

// Map style accessors
static const std::unordered_map<String, DeviceFactory> registry = {

    // clang-format off
    {OSSM_SERVICE_ID, []() -> Device * { return new OSSM(); }},
    {DOMI_SERVICE_ID, []() -> Device * { return new Domi2(); }}
    // clang-format on
};

inline const DeviceFactory *getDeviceFactory(const String &serviceId)
{
    auto it = registry.find(serviceId);
    if (it == registry.end())
        return nullptr;
    return &it->second;
}

#endif