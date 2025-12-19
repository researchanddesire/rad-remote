#include "utils.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/// @brief Reads and parses a JSON file from LittleFS
/// @param filename The filename to read
/// @param doc The JsonDocument to populate
/// @return true if successful, false otherwise
bool readJsonFile(const String& filename, JsonDocument& doc) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        ESP_LOGE("BUTTPLUGIO", "Failed to open file: %s", filename.c_str());
        return false;
    }

    String jsonString = file.readString();
    vTaskDelay(1);
    file.close();

    DeserializationError error = deserializeJson(doc, jsonString);
    vTaskDelay(1);
    if (error) {
        ESP_LOGE("BUTTPLUGIO", "Failed to parse file %s: %s", filename.c_str(),
                 error.c_str());
        return false;
    }

    return true;
}

/// @brief Validates that a config document has the required structure
/// @param configDoc The configuration document to validate
/// @param filename The filename for error reporting
/// @return true if valid, false otherwise
bool validateConfigStructure(const JsonDocument& configDoc,
                             const String& filename) {
    if (configDoc["communication"].isNull()) {
        ESP_LOGE("BUTTPLUGIO",
                 "Communication object not found in config file: %s",
                 filename.c_str());
        return false;
    }

    JsonArrayConst communicationArray = configDoc["communication"];
    if (communicationArray.isNull() || communicationArray.size() == 0) {
        ESP_LOGE("BUTTPLUGIO",
                 "Communication array is empty in config file: %s",
                 filename.c_str());
        return false;
    }

    JsonObjectConst communication = communicationArray[0];
    if (communication["btle"].isNull()) {
        ESP_LOGE("BUTTPLUGIO", "Btle object not found in config file: %s",
                 filename.c_str());
        return false;
    }

    JsonObjectConst btleData = communication["btle"];
    if (btleData.isNull()) {
        ESP_LOGE("BUTTPLUGIO", "Btle data is null in config file: %s",
                 filename.c_str());
        return false;
    }

    JsonArrayConst names = btleData["names"];
    if (names.isNull()) {
        ESP_LOGE("BUTTPLUGIO", "Names array not found in config file: %s",
                 filename.c_str());
        return false;
    }

    return true;
}

/// @brief Checks if a device name matches any pattern in the names array
/// @param deviceName The device name to match
/// @param names The array of name patterns to check against
/// @return true if a match is found, false otherwise
bool matchesDeviceName(const String& deviceName, const JsonArrayConst& names) {
    for (JsonString name : names) {
        String nameString = String(name.c_str());
        nameString.replace("*", ".*");

        String regexPattern = "^" + nameString + "$";
        std::regex re(regexPattern.c_str());

        if (std::regex_match(deviceName.c_str(), re)) {
            ESP_LOGI("BUTTPLUGIO", "Pattern matched advertised device: %s",
                     name.c_str());
            vTaskDelay(1);
            return true;
        }
        vTaskDelay(1);
    }
    return false;
}

/// @brief Extracts characteristics for a service UUID from a config document
/// @param configDoc The configuration document
/// @param serviceUUID The service UUID to extract characteristics for
/// @return JsonObject containing the characteristics, or null if not found
JsonObjectConst extractCharacteristics(const JsonDocument& configDoc,
                                       const std::string& serviceUUID) {
    JsonObjectConst communication = configDoc["communication"][0];
    JsonObjectConst btleData = communication["btle"];
    JsonObjectConst services = btleData["services"];

    if (services.isNull() || services[serviceUUID].isNull()) {
        ESP_LOGE("BUTTPLUGIO", "Service UUID %s not found in services",
                 serviceUUID.c_str());
        return JsonObject();
    }

    JsonObjectConst characteristics = services[serviceUUID];

    String characteristicsString = "";
    serializeJson(characteristics, characteristicsString);
    vTaskDelay(1);
    ESP_LOGI("BUTTPLUGIO", "Characteristics: %s",
             characteristicsString.c_str());

    return characteristics;
}
