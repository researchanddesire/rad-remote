#ifndef LOVENSE_DEVICE_HPP
#define LOVENSE_DEVICE_HPP

#include <Arduino.h>

#include <devices/device.h>

#include "data.hpp"

class LovenseDevice : public Device {
  private:
    JsonDocument config;

  public:
    LovenseDevice(const NimBLEAdvertisedDevice *advertisedDevice)
        : Device(advertisedDevice) {
        // get the type of the device

        JsonDocument doc;
        DeserializationError error =
            deserializeJson(doc, service_characteristics);

        if (error) {
            ESP_LOGE("LOVENSE", "JSON parse failed: %s", error.c_str());
            return;
        }

        // use the service_characteristics to set the "tx" and "rx"
        // characteristics
        auto service = advertisedDevice->getServiceUUID().toString();

        if (!doc.containsKey(service)) {
            ESP_LOGE("LOVENSE", "Service not found: %s", service.c_str());
            return;
        }

        String tx = doc[service]["tx"].as<String>();
        String rx = doc[service]["rx"].as<String>();

        const auto notifyCallback =
            [this](NimBLERemoteCharacteristic *pRemoteCharacteristic,
                   uint8_t *pData, size_t length, bool isNotify) {
                rxValue = String(reinterpret_cast<char *>(pData), length);
                ESP_LOGD("LOVENSE", "Notification received, value: %s",
                         rxValue.c_str());
            };

        characteristics = {
            {"tx", {NimBLEUUID(tx.c_str())}},
            {"rx", DeviceCharacteristics{NimBLEUUID(rx.c_str()),
                                         .notifyCallback = notifyCallback}}};

        // parse the config.
        error = deserializeJson(config, buttplugIO_config_lovense);
        if (error) {
            ESP_LOGE("LOVENSE", "JSON parse failed: %s", error.c_str());
            return;
        }
        ESP_LOGI("LOVENSE", "JSON parse successful");
    }

    String rxValue = "";

    void onConnect() override {
        rxValue = "";
        // This is required to parse the config file.
        // Resend "DeviceType;" every 250ms until rxValue is set
        const TickType_t checkInterval = 250 / portTICK_PERIOD_MS;
        TickType_t lastSendTick = xTaskGetTickCount();

        do {
            vTaskDelay(
                50 /
                portTICK_PERIOD_MS);  // check frequently for rxValue updates

            // Every 250ms, resend the command if rxValue not set
            TickType_t currentTick = xTaskGetTickCount();
            if ((currentTick - lastSendTick) >= checkInterval) {
                send("tx", "DeviceType;");
                lastSendTick = currentTick;
            }

        } while (rxValue.isEmpty());

        auto deviceType = rxValue;
        String firstLetter = deviceType;
        int colonIndex = deviceType.indexOf(':');
        if (colonIndex != -1) {
            firstLetter = deviceType.substring(0, colonIndex);
        } else {
            firstLetter = deviceType.substring(0, 1);
        }

        JsonDocument doc;
        DeserializationError error =
            deserializeJson(doc, buttplugIO_config_lovense);
        if (error) {
            ESP_LOGE("LOVENSE", "JSON parse failed: %s", error.c_str());
            return;
        }

        // get the configuration for the device type
        JsonArray configurations = doc["configurations"];
        if (configurations.isNull()) {
            ESP_LOGE("LOVENSE", "Configuration not found: %s",
                     firstLetter.c_str());
            return;
        }

        JsonObject defaults = doc["defaults"];
        if (defaults.isNull()) {
            ESP_LOGE("LOVENSE", "Defaults not found");
            return;
        }

        JsonArray features = JsonArray();

        bool found = false;
        // For each configuration, print its JSON string
        for (JsonObject configuration : configurations) {
            JsonArray identifier = configuration["identifier"];

            for (String item : identifier) {
                if (item.compareTo(firstLetter) == 0) {
                    ESP_LOGI("LOVENSE", "Found configuration:");
                    features = configuration["features"];

                    if (features.isNull()) {
                        features = defaults["features"];
                    }

                    found = true;
                    break;
                }
            }

            if (found) break;
        }

        // get the features for the configuration, but we should always have the
        // defaults.
        if (features.isNull()) {
            ESP_LOGE("LOVENSE", "Features not found: %s", firstLetter.c_str());
            return;
        }

        String featuresString = "";
        serializeJson(features, featuresString);
        ESP_LOGI("LOVENSE", "Features: %s", featuresString.c_str());
    }

    NimBLEUUID getServiceUUID() override {
        return advertisedDevice->getServiceUUID();
    }
    const char *getName() override { return "Lovense"; }
};

#endif  // LOVENSE_DEVICE_HPP