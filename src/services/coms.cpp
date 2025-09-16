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

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks
{
    void onConnect(NimBLEClient *pClient) override
    {
        ESP_LOGI(TAG, "Connected");
    }

    void onDisconnect(NimBLEClient *pClient, int reason) override
    {
        ESP_LOGI(TAG, "%s Disconnected, reason = %d - Starting scan", pClient->getPeerAddress().toString().c_str(), reason);
        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    }

    /********************* Security handled here *********************/
    void onPassKeyEntry(NimBLEConnInfo &connInfo) override
    {
        ESP_LOGI(TAG, "Server Passkey Entry");
        /**
         * This should prompt the user to enter the passkey displayed
         * on the peer device.
         */
        NimBLEDevice::injectPassKey(connInfo, 123456);
    }

    void onConfirmPasskey(NimBLEConnInfo &connInfo, uint32_t pass_key) override
    {
        ESP_LOGI(TAG, "The passkey YES/NO number: %" PRIu32, pass_key);
        /** Inject false if passkeys don't match. */
        NimBLEDevice::injectConfirmPasskey(connInfo, true);
    }

    /** Pairing process complete, we can check the results in connInfo */
    void onAuthenticationComplete(NimBLEConnInfo &connInfo) override
    {
        if (!connInfo.isEncrypted())
        {
            ESP_LOGE(TAG, "Encrypt connection failed - disconnecting");
            /** Find the client with the connection handle provided in connInfo */
            NimBLEDevice::getClientByHandle(connInfo.getConnHandle())->disconnect();
            return;
        }
    }
} clientCallbacks;

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
    NimBLEDevice::init("NimBLE-Client");

    /**
     * Set the IO capabilities of the device, each option will trigger a different pairing method.
     *  BLE_HS_IO_KEYBOARD_ONLY   - Passkey pairing
     *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
     *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
     */
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); // use passkey
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison

    /**
     * 2 different ways to set security - both calls achieve the same result.
     *  no bonding, no man in the middle protection, BLE secure connections.
     *  These are the default values, only shown here for demonstration.
     */
    // NimBLEDevice::setSecurityAuth(false, false, true);
    // NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM | BLE_SM_PAIR_AUTHREQ_SC);

    /** Optional: set the transmit power */
    NimBLEDevice::setPower(3); /** 3dbm */
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

    // xTaskCreatePinnedToCore(
    //     [](void *pvParameter)
    //     {
    //         vTaskDelay(1000);
    //         while (true)
    //         {
    //             vTaskDelay(100);

    //             if (doConnect)
    //             {
    //                 doConnect = false;
    //                 /** Found a device we want to connect to, do it now */
    //                 if (connectToServer())
    //                 {
    //                     ESP_LOGI(TAG, "Success! we should now be getting notifications, scanning for more!");
    //                 }
    //                 else
    //                 {
    //                     ESP_LOGE(TAG, "Failed to connect, starting scan");
    //                     NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    //                 }
    //             }

    //             // if we're not connected continue.
    //             const NimBLEAdvertisedDevice *currentAdv = advDevice;
    //             if (currentAdv == nullptr)
    //             {
    //                 ESP_LOGV(TAG, "No advertised device available yet");
    //                 continue;
    //             }

    //             NimBLEClient *client = NimBLEDevice::getClientByPeerAddress(currentAdv->getAddress());
    //             if (client == nullptr)
    //             {
    //                 ESP_LOGV(TAG, "No client instance for peer: %s", currentAdv->getAddress().toString().c_str());
    //                 continue;
    //             }

    //             if (!client->isConnected())
    //             {
    //                 ESP_LOGV(TAG, "Client exists but not connected to: %s", currentAdv->getAddress().toString().c_str());
    //                 continue;
    //             }

    //             // first check for changes in lastSettings
    //             if (lastSettings.speed != settings.speed)
    //             {
    //                 sendCommand("set:speed:" + String(settings.speed, 0));
    //                 lastSettings.speed = settings.speed;
    //                 ESP_LOGI(TAG, "Speed changed, sending command: set:speed:%d", settings.speed);
    //             }

    //             if (lastSettings.stroke != settings.stroke)
    //             {
    //                 sendCommand("set:stroke:" + String(settings.stroke, 0));
    //                 lastSettings.stroke = settings.stroke;
    //                 ESP_LOGI(TAG, "Stroke changed, sending command: set:stroke:%d", settings.stroke);
    //             }

    //             if (lastSettings.sensation != settings.sensation)
    //             {
    //                 sendCommand("set:sensation:" + String(settings.sensation, 0));
    //                 lastSettings.sensation = settings.sensation;
    //                 ESP_LOGI(TAG, "Sensation changed, sending command: set:sensation:%d", settings.sensation);
    //             }

    //             if (lastSettings.depth != settings.depth)
    //             {
    //                 sendCommand("set:depth:" + String(settings.depth, 0));
    //                 lastSettings.depth = settings.depth;
    //                 ESP_LOGI(TAG, "Depth changed, sending command: set:depth:%d", settings.depth);
    //             }

    //             if (lastSettings.pattern != settings.pattern)
    //             {
    //                 sendCommand("set:pattern:" + String(static_cast<uint8_t>(settings.pattern), 0));
    //                 lastSettings.pattern = settings.pattern;
    //                 ESP_LOGI(TAG, "Pattern changed, sending command: set:pattern:%d", static_cast<uint8_t>(settings.pattern));
    //             }

    //             // if there's nothing in the queue, continue.
    //             if (commandQueue.empty())
    //             {
    //                 continue;
    //             }

    //             // if there's something in the queue, send it.
    //             String command = commandQueue.front();
    //             commandQueue.pop();
    //             ESP_LOGI(TAG, "Sending command: %s", command.c_str());
    //             NimBLERemoteService *svc = client->getService(OSSM_SERVICE_UUID);
    //             if (svc == nullptr)
    //             {
    //                 ESP_LOGW(TAG, "OSSM service not found on peer; dropping command");
    //                 continue;
    //             }

    //             NimBLERemoteCharacteristic *pChr = svc->getCharacteristic(OSSM_CHARACTERISTIC_UUID);
    //             if (pChr == nullptr)
    //             {
    //                 ESP_LOGW(TAG, "OSSM characteristic not found on peer; dropping command");
    //                 continue;
    //             }

    //             if (!pChr->canWrite())
    //             {
    //                 ESP_LOGW(TAG, "OSSM characteristic not writable; dropping command");
    //                 continue;
    //             }

    //             if (pChr->writeValue(command))
    //             {
    //                 ESP_LOGI(TAG, "Command sent: %s", command.c_str());
    //             }
    //             else
    //             {
    //                 ESP_LOGE(TAG, "Failed to write command to OSSM characteristic");
    //             }
    //         }
    //     },
    //     "BLE Task",
    //     10 * configMINIMAL_STACK_SIZE,
    //     NULL,
    //     1,
    //     NULL,
    //     1);
}
