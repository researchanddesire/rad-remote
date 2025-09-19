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

#define TAG "COMS"

static const NimBLEAdvertisedDevice *advDevice;
static bool doConnect = false;
static uint32_t scanTimeMs = 0; /** scan time in milliseconds, 0 = scan forever */

static std::queue<String> commandQueue;

Device *device;

/** Define a class to handle the callbacks when scan events are received */
class ScanCallbacks : public NimBLEScanCallbacks
{
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override
    {
        // get all service UUIDs
        const DeviceFactory *factory = nullptr;
        auto countOfServiceUUIDs = advertisedDevice->getServiceUUIDCount();
        for (int i = 0; i < countOfServiceUUIDs; i++)
        {
            vTaskDelay(1);
            auto serviceUUID = advertisedDevice->getServiceUUID(i);

            auto name = advertisedDevice->getName();

            // check if the service UUID is in the registry
            factory = getDeviceFactory(serviceUUID);

            if (factory != nullptr)
            {
                break;
            }
        }

        if (!factory)
        {
            return;
        }

        NimBLEDevice::getScan()->stop();

        /** stop scan before connecting */
        /** Save the device reference in a global for the client to use*/
        advDevice = advertisedDevice;
        device = (*factory)(advertisedDevice);

        return;
    }
} scanCallbacks;

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    std::string str = (isNotify == true) ? "Notification" : "Indication";
    str += " from ";
    str += pRemoteCharacteristic->getClient()->getPeerAddress().toString();
    str += ": Service = " + pRemoteCharacteristic->getRemoteService()->getUUID().toString();
    str += ", Characteristic = " + pRemoteCharacteristic->getUUID().toString();
    str += ", Value = " + std::string((char *)pData, length);

    ESP_LOGV(TAG, "%s", str.c_str());
}

/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer()
{
    if (advDevice == nullptr)
    {
        ESP_LOGE(TAG, "connectToServer called with null advDevice");
        return false;
    }

    return true;
}

void sendCommand(const String &command)
{
    // Only allow commands of the form: set:depth|sensation|pattern|speed|stroke:0-100
    // Example: set:speed:75

    // Regex: ^set:(depth|sensation|pattern|speed|stroke):([0-9]{1,3})$
    // Value must be 0-100

    std::regex cmdRegex("^set:(depth|sensation|pattern|speed|stroke):([0-9]{1,3})$");
    std::cmatch match;
    if (std::regex_match(command.c_str(), match, cmdRegex))
    {
        int value = atoi(match[2].str().c_str());
        if (value >= 0 && value <= 100)
        {
            std::string cmdType = match[1].str();

            // Remove any previous commands of the same type from the queue
            std::queue<String> tempQueue;
            while (!commandQueue.empty())
            {
                String queuedCmd = commandQueue.front();
                commandQueue.pop();

                std::cmatch queuedMatch;
                if (std::regex_match(queuedCmd.c_str(), queuedMatch, cmdRegex))
                {
                    std::string queuedType = queuedMatch[1].str();
                    if (queuedType == cmdType)
                    {
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
    ESP_LOGW(TAG, "Invalid command format: %s", command.c_str());
}

void initBLE()
{
    ESP_LOGI(TAG, "Starting NimBLE Client");

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("OSSM-REMOTE");

    /** Set the transmit power to maximum (9 dBm for ESP32) */
    NimBLEDevice::setPower(9); /** 9dBm */
    NimBLEScan *pScan = NimBLEDevice::getScan();

    /** Set the callbacks to call when scan events occur, no duplicates */
    pScan->setScanCallbacks(&scanCallbacks, false);

    /** Set scan interval (how often) and window (how long) in milliseconds */
    pScan->setInterval(1000);
    pScan->setWindow(1000);

    /**
     * Active scan will gather scan response data from advertisers
     *  but will use more energy from both devices
     */
    pScan->setActiveScan(false);

    /** Start scanning for advertisers */
    pScan->start(scanTimeMs);
    ESP_LOGI(TAG, "Scanning for peripherals");
}
