#ifndef BUTTPLUGIO_DEVICE_HPP
#define BUTTPLUGIO_DEVICE_HPP

#include <Arduino.h>

#include <LittleFS.h>
#include <regex>

#include "../device.h"
#include "../lovense/LovenseDevice.hpp"

/// @brief A device factory for devices that use the ButtplugIO protocol.
auto ButtplugIODeviceFactory =
    [](const NimBLEAdvertisedDevice *advertisedDevice) -> Device * {
    delay(2000);

    auto serviceUUID = advertisedDevice->getServiceUUID().toString();
    String deviceName = String(advertisedDevice->getName().c_str());

    JsonObject characteristics;

    // use littleFS to read the "registry.json" file
    File file = LittleFS.open("/registry.json", "r");
    if (!file) {
        ESP_LOGE("BUTTPLUGIO", "Failed to open registry.json");
        return nullptr;
    }

    String jsonString = file.readString();
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        ESP_LOGE("BUTTPLUGIO", "Failed to parse registry.json: %s",
                 error.c_str());
        return nullptr;
    }

    if (!doc.containsKey(serviceUUID)) {
        ESP_LOGE("BUTTPLUGIO", "Service UUID not found in registry.json: %s",
                 serviceUUID.c_str());
        return nullptr;
    }

    // next up we should figure out how many possible files there are to read.
    JsonArray configFiles = doc[serviceUUID];
    if (configFiles.isNull()) {
        ESP_LOGE("BUTTPLUGIO", "Config files not found in registry.json: %s",
                 serviceUUID.c_str());
        return nullptr;
    }

    JsonObject configObject = doc[serviceUUID];

    bool foundConfigFile = false;

    for (JsonString configFileRow : configFiles) {
        auto configFileName = configFileRow.c_str();
        File configFile = LittleFS.open(configFileName, "r");
        if (!configFile) {
            ESP_LOGE("BUTTPLUGIO", "Failed to open config file: %s",
                     configFileName);
            configFile.close();

            continue;
        }

        ESP_LOGI("BUTTPLUGIO", "Opened config file: %s", configFileName);

        // JSON parse the config file
        JsonDocument configDoc;
        DeserializationError error = deserializeJson(configDoc, configFile);
        if (error) {
            ESP_LOGE("BUTTPLUGIO", "Failed to parse config file: %s",
                     configFileName);
            configFile.close();
            continue;
        }
        configFile.close();

        // json document should have a "communication" object
        if (!configDoc.containsKey("communication")) {
            ESP_LOGE("BUTTPLUGIO",
                     "Communication object not found in config file: %s",
                     configFileName);
            configFile.close();
            continue;
        }

        if (!configDoc.containsKey("communication")) {
            ESP_LOGE("BUTTPLUGIO",
                     "Communication object not found in config file: %s",
                     configFileName);
            configFile.close();
            continue;
        }

        JsonObject communication = configDoc["communication"][0];

        // json document should have a "btle" object... assume the first item in
        // communication array is correct. this might be a problem in the
        // future.
        if (!communication.containsKey("btle")) {
            ESP_LOGE("BUTTPLUGIO", "Btle object not found in config file: %s",
                     configFileName);
            configFile.close();
            continue;
        }

        // get array of services
        JsonObject btleData = communication["btle"];
        if (btleData.isNull()) {
            ESP_LOGE("BUTTPLUGIO",
                     "Services object not found in config file: %s",
                     configFileName);
            configFile.close();
            continue;
        }

        // print names array
        JsonArray names = btleData["names"];
        if (names.isNull()) {
            ESP_LOGE("BUTTPLUGIO", "Names array not found in config file: %s",
                     configFileName);
            configFile.close();
            continue;
        }

        // do any of the names match the advertised device name?
        for (JsonString name : names) {
            String nameString = String(name.c_str());
            nameString.replace("*", ".*");

            String regexPattern = "^" + nameString + "$";

            std::regex re(regexPattern.c_str());
            if (std::regex_match(deviceName.c_str(), re)) {
                ESP_LOGI("BUTTPLUGIO", "Pattern matched advertised device: %s",
                         name.c_str());
                foundConfigFile = true;
                break;
            }
        }

        if (foundConfigFile) {
            // save the
            characteristics = btleData["services"][serviceUUID];

            String characteristicsString = "";
            serializeJson(characteristics, characteristicsString);
            ESP_LOGI("BUTTPLUGIO", "Characteristics: %s",
                     characteristicsString.c_str());

            if (characteristics.isNull()) {
                ESP_LOGE("BUTTPLUGIO",
                         "Characteristics not found in config file: %s",
                         configFileName);
                configFile.close();
                continue;
            }

            break;
        }
    }

    String characteristicsString = "";
    serializeJson(characteristics, characteristicsString);
    ESP_LOGI("BUTTPLUGIO", "Characteristics: %s",
             characteristicsString.c_str());

    delay(10000);

    return new LovenseDevice(advertisedDevice);
};

#endif  // BUTTPLUGIO_DEVICE_HPP