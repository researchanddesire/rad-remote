#include "coms.h"

/** NimBLE_Client Demo:
 *
 *  Demonstrates many of the available features of the NimBLE client library.
 *
 *  Created: on March 24 2020
 *      Author: H2zero
 */

#include <Arduino.h>

#include <NimBLEDevice.h>
#include <esp_log.h>
#include <queue>
#include <regex>

static const char *TAG_COMS = "COMS";

const NimBLEAdvertisedDevice *advDevice;
static bool doConnect = false;
static uint32_t scanTimeMs =
    0; /** scan time in milliseconds, 0 = scan forever */

static std::queue<String> commandQueue;

std::vector<DiscoveredDevice> discoveredDevices;

/** Define a class to handle the callbacks when scan events are received */
class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override {
        // get all service UUIDs
        const DeviceFactory *factory = nullptr;
        auto countOfServiceUUIDs = advertisedDevice->getServiceUUIDCount();
        for (int i = 0; i < countOfServiceUUIDs; i++) {
            vTaskDelay(1);
            auto serviceUUID = advertisedDevice->getServiceUUID(i);

            auto name = advertisedDevice->getName();

            // print the service UUID and name
            ESP_LOGI("DEBUG_FOLLOWER", "Service UUID: %s, Name: %s",
                     serviceUUID.toString().c_str(), name.c_str());

            // check if the service UUID is in the registry
            factory = getDeviceFactory(serviceUUID);

            if (factory != nullptr) {
                break;
            }
        }

        if (!factory) {
            return;
        }

        // Check if this device is already in our discovered list
        std::string deviceAddress = advertisedDevice->getAddress().toString();
        for (auto &discovered : discoveredDevices) {
            if (discovered.advertisedDevice->getAddress().toString() ==
                deviceAddress) {
                // Device already discovered, update RSSI if it's stronger
                if (advertisedDevice->getRSSI() > discovered.rssi) {
                    discovered.rssi = advertisedDevice->getRSSI();
                }
                return;
            }
        }

        // New device found - add to list
        DiscoveredDevice newDevice;
        newDevice.advertisedDevice = advertisedDevice;
        newDevice.factory = factory;
        newDevice.displayName = advertisedDevice->getName().c_str();
        newDevice.rssi = advertisedDevice->getRSSI();

        discoveredDevices.push_back(newDevice);

        ESP_LOGI(TAG_COMS,
                 "Discovered device: %s (RSSI: %d) - Total devices: %d",
                 newDevice.displayName.c_str(), newDevice.rssi,
                 discoveredDevices.size());

        // Scan continues - UI will update in real-time as devices are found

        return;
    }

    void onScanEnd(const NimBLEScanResults &results, int reason) override {
        ESP_LOGI(TAG_COMS,
                 "Scan timeout reached. Found %d compatible device(s)",
                 discoveredDevices.size());
    }

} scanCallbacks;

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData,
              size_t length, bool isNotify) {
    std::string str = (isNotify == true) ? "Notification" : "Indication";
    str += " from ";
    str += pRemoteCharacteristic->getClient()->getPeerAddress().toString();
    str += ": Service = " +
           pRemoteCharacteristic->getRemoteService()->getUUID().toString();
    str += ", Characteristic = " + pRemoteCharacteristic->getUUID().toString();
    str += ", Value = " + std::string((char *)pData, length);

    ESP_LOGV(TAG_COMS, "%s", str.c_str());
}

/** Handles the provisioning of clients and connects / interfaces with the
 * server */
bool connectToServer() {
    if (advDevice == nullptr) {
        ESP_LOGE(TAG_COMS, "connectToServer called with null advDevice");
        return false;
    }

    return true;
}

void sendCommand(const String &command) {
    // Only allow commands of the form:
    // set:depth|sensation|pattern|speed|stroke:0-100 Example: set:speed:75

    // Regex: ^set:(depth|sensation|pattern|speed|stroke):([0-9]{1,3})$
    // Value must be 0-100

    std::regex cmdRegex(
        "^set:(depth|sensation|pattern|speed|stroke):([0-9]{1,3})$");
    std::cmatch match;
    if (std::regex_match(command.c_str(), match, cmdRegex)) {
        int value = atoi(match[2].str().c_str());
        if (value >= 0 && value <= 100) {
            std::string cmdType = match[1].str();

            // Remove any previous commands of the same type from the queue
            std::queue<String> tempQueue;
            while (!commandQueue.empty()) {
                String queuedCmd = commandQueue.front();
                commandQueue.pop();

                std::cmatch queuedMatch;
                if (std::regex_match(queuedCmd.c_str(), queuedMatch,
                                     cmdRegex)) {
                    std::string queuedType = queuedMatch[1].str();
                    if (queuedType == cmdType) {
                        // Skip this command (removes previous of same type)
                        continue;
                    }
                }
                tempQueue.push(queuedCmd);
            }
            // Restore the filtered queue
            commandQueue = tempQueue;

            // Add the new command
            commandQueue.push(command);
            return;
        }
    }
    ESP_LOGW(TAG_COMS, "Invalid command format: %s", command.c_str());
}

void clearDiscoveredDevices() {
    discoveredDevices.clear();
    ESP_LOGI(TAG_COMS, "Cleared discovered devices list");
}

void initBLE() {
    ESP_LOGI(TAG_COMS, "Starting NimBLE Client");

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("OSSM-REMOTE");

    /** Set the transmit power to maximum (9 dBm for ESP32) */
    NimBLEDevice::setPower(9); /** 9dBm */
    NimBLEScan *pScan = NimBLEDevice::getScan();

    /** Set the callbacks to call when scan events occur, no duplicates */
    pScan->setScanCallbacks(&scanCallbacks, false);

    /** Set scan interval (how often) and window (how long) in milliseconds */
    pScan->setInterval(100);
    pScan->setWindow(100);

    /**
     * Active scan will gather scan response data from advertisers
     *  but will use more energy from both devices
     */
    pScan->setActiveScan(true);

    /** Clear any previously discovered devices */
    clearDiscoveredDevices();

    /** Start scanning for 10 seconds to discover devices */
    pScan->start(10, false, false);
    ESP_LOGI(TAG_COMS, "Scanning for peripherals (10 second scan)");
}
