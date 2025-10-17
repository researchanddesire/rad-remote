#ifndef BUTTPLUGIO_UTILS_HPP
#define BUTTPLUGIO_UTILS_HPP

#pragma once

#include <Arduino.h>

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <regex>

/// @brief Reads and parses a JSON file from LittleFS
/// @param filename The filename to read
/// @param doc The JsonDocument to populate
/// @return true if successful, false otherwise
bool readJsonFile(const String& filename, JsonDocument& doc);

/// @brief Validates that a config document has the required structure
/// @param configDoc The configuration document to validate
/// @param filename The filename for error reporting
/// @return true if valid, false otherwise
bool validateConfigStructure(const JsonDocument& configDoc,
                             const String& filename);

/// @brief Checks if a device name matches any pattern in the names array
/// @param deviceName The device name to match
/// @param names The array of name patterns to check against
/// @return true if a match is found, false otherwise
bool matchesDeviceName(const String& deviceName, const JsonArrayConst& names);

/// @brief Extracts characteristics for a service UUID from a config document
/// @param configDoc The configuration document
/// @param serviceUUID The service UUID to extract characteristics for
/// @return JsonObject containing the characteristics, or null if not found
JsonObjectConst extractCharacteristics(const JsonDocument& configDoc,
                                       const std::string& serviceUUID);

#endif  // BUTTPLUGIO_UTILS_HPP