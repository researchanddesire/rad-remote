#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include <Arduino.h>

#include <NimBLEUUID.h>
#include <string>
#include <unordered_map>

#include "device.h"
#include "lovense/LovenseDevice.hpp"
#include "lovense/data.hpp"
#include "lovense/domi/domi_device.hpp"
#include "researchAndDesire/ossm/ossm_device.hpp"
#include "serviceUUIDs.h"

// Factory function type for creating device instances
typedef Device *(*DeviceFactory)(
    const NimBLEAdvertisedDevice *advertisedDevice);

// Lazy-initialized map between uppercase service UUIDs and device factories
inline const std::unordered_map<std::string, DeviceFactory> &getRegistry() {
    static const std::unordered_map<std::string, DeviceFactory> registry =
        []() {
            std::unordered_map<std::string, DeviceFactory> map;

            // Known explicit services
            map.emplace(
                OSSM_SERVICE_ID,
                [](const NimBLEAdvertisedDevice *advertisedDevice) -> Device * {
                    return new OSSM(advertisedDevice);
                });
            // map.emplace(
            //     DOMI_SERVICE_ID,
            //     [](const NimBLEAdvertisedDevice *advertisedDevice) -> Device
            //     * {
            //         return new Domi2(advertisedDevice);
            //     });

            // Dynamically add all Lovense advertised services
            const size_t serviceCount =
                sizeof(advertised_services) / sizeof(advertised_services[0]);
            for (size_t i = 0; i < serviceCount; ++i) {
                std::string uuidStr = advertised_services[i];
                std::transform(uuidStr.begin(), uuidStr.end(), uuidStr.begin(),
                               ::toupper);
                map.emplace(uuidStr,
                            [](const NimBLEAdvertisedDevice *advertisedDevice)
                                -> Device * {
                                return new LovenseDevice(advertisedDevice);
                            });
            }

            return map;
        }();
    return registry;
}

inline const DeviceFactory *getDeviceFactory(const NimBLEUUID &serviceUUID) {
    // Convert NimBLEUUID to Uppercase std::string for lookup
    std::string uuidStr = serviceUUID.toString().c_str();
    std::transform(uuidStr.begin(), uuidStr.end(), uuidStr.begin(), ::toupper);

    const auto &registry = getRegistry();
    auto it = registry.find(uuidStr);

    if (it == registry.end()) return nullptr;

    return &it->second;
}

#endif