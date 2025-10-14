#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include <Arduino.h>

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <NimBLEUUID.h>
#include <string>
#include <unordered_map>

#include "buttplugio/buttplugIODevice.hpp"
#include "device.h"
#include "lovense/LovenseDevice.hpp"
#include "lovense/data.hpp"
#include "lovense/domi/domi_device.hpp"
#include "researchAndDesire/ossm/ossm_device.hpp"
#include "serviceUUIDs.h"

static const char *REGISTRY_TAG = "REGISTRY";

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

            // Try to read registry.json from LittleFS

            if (LittleFS.exists("/registry.json")) {
                vTaskDelay(1);
                File file = LittleFS.open("/registry.json", "r");
                vTaskDelay(1);
                if (file) {
                    size_t fileSize = file.size();
                    if (fileSize > 0) {
                        vTaskDelay(1);
                        String jsonString = file.readString();
                        file.close();

                        JsonDocument doc;
                        DeserializationError error =
                            deserializeJson(doc, jsonString);
                        vTaskDelay(1);

                        if (!error) {
                            ESP_LOGI(REGISTRY_TAG,
                                     "Loaded device registry from LittleFS");

                            // Parse JSON and add UUIDs to registry
                            for (JsonPair pair : doc.as<JsonObject>()) {
                                std::string uuidStr = pair.key().c_str();
                                std::transform(uuidStr.begin(), uuidStr.end(),
                                               uuidStr.begin(), ::toupper);

                                map.emplace(uuidStr, ButtplugIODeviceFactory);
                                vTaskDelay(1);
                            }
                        } else {
                            ESP_LOGW(REGISTRY_TAG,
                                     "Failed to parse registry.json: %s",
                                     error.c_str());
                        }
                    } else {
                        ESP_LOGW(REGISTRY_TAG,
                                 "registry.json file too large or empty");
                        file.close();
                    }
                } else {
                    ESP_LOGW(REGISTRY_TAG, "Failed to open registry.json");
                }
            } else {
                ESP_LOGD(REGISTRY_TAG, "registry.json not found in LittleFS");
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

    if (it == registry.end()) {
        // For development: always return LovenseDevice factory for unknown
        // UUIDs
        ESP_LOGD(REGISTRY_TAG,
                 "Unknown service UUID %s, using LovenseDevice for development",
                 uuidStr.c_str());
        static DeviceFactory lovenseFactory =
            [](const NimBLEAdvertisedDevice *advertisedDevice) -> Device * {
            return new LovenseDevice(advertisedDevice);
        };
        return &lovenseFactory;
    }

    return &it->second;
}

#endif