#include <ArduinoJson.h>
#include <NimBLEUUID.h>

#include "utils.h"

class ButtplugIoProtocol {
  protected:
    NimBLEUUID serviceUUID;
    JsonDocument config;
    JsonDocument protocol;
    String configFileName;
    String deviceType;
    String deviceName;

  public:
    // Constructor accepting the service UUID and config
    ButtplugIoProtocol(const String& configFileName,
                       const JsonObjectConst& characteristicsConfig)
        : serviceUUID(serviceUUID), config() {
        // Copy the characteristics data into our own config document
        // to avoid lifetime issues with the original JsonObjectConst
        config.set(characteristicsConfig);
        this->configFileName = configFileName;
    }

    virtual String getIdentifier() = 0;

    void setProtocol(const String& identifierString) {
        // read the config file
        JsonDocument doc;
        if (!readJsonFile(configFileName, doc)) {
            ESP_LOGE("BUTTPLUGIO_PROTOCOL", "Failed to read config file: %s",
                     configFileName.c_str());
            return;
        }

        // get the configuration for the device type
        JsonArray configurations = doc["configurations"];
        if (configurations.isNull()) {
            ESP_LOGE("BUTTPLUGIO_PROTOCOL", "Configuration not found: %s",
                     identifierString.c_str());
            return;
        }

        JsonObject defaults = doc["defaults"];
        if (defaults.isNull()) {
            ESP_LOGE("BUTTPLUGIO_PROTOCOL", "Defaults not found");
            return;
        }

        JsonArray features = JsonArray();

        bool found = false;
        // For each configuration, print its JSON string
        for (JsonObject configuration : configurations) {
            JsonArray identifierArray = configuration["identifier"];

            for (String item : identifierArray) {
                if (item.compareTo(identifierString) == 0) {
                    ESP_LOGI("BUTTPLUGIO_PROTOCOL", "Found configuration:");
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
            ESP_LOGE("BUTTPLUGIO_PROTOCOL", "Features not found: %s",
                     identifierString.c_str());
            return;
        }

        String featuresString = "";
        serializeJson(features, featuresString);
        ESP_LOGI("BUTTPLUGIO_PROTOCOL", "Features: %s", featuresString.c_str());
    }
};
