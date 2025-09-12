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
static const char *TAG = "COMS";

static const NimBLEAdvertisedDevice *advDevice;
static bool doConnect = false;
static uint32_t scanTimeMs = 0; /** scan time in milliseconds, 0 = scan forever */

static std::queue<String> commandQueue;

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
        vTaskDelay(1); // Say hello to the watchdog <3
        ESP_LOGV(TAG, "Advertised Device found: %s", advertisedDevice->toString().c_str());
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(OSSM_SERVICE_UUID)) ||
            advertisedDevice->getName() == OSSM_DEVICE_NAME)
        {
            ESP_LOGI(TAG, "Found Our Service or Device Name");
            /** stop scan before connecting */
            NimBLEDevice::getScan()->stop();
            /** Save the device reference in a global for the client to use*/
            advDevice = advertisedDevice;
            /** Ready to connect now */
            doConnect = true;
        }
    }

    /** Callback to process the results of the completed scan or restart it */
    void onScanEnd(const NimBLEScanResults &results, int reason) override
    {
        ESP_LOGI(TAG, "Scan Ended, reason: %d, device count: %d", reason, results.getCount());
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

    NimBLEClient *pClient = nullptr;

    /** Check if we have a client we should reuse first **/
    if (NimBLEDevice::getCreatedClientCount())
    {
        ESP_LOGD(TAG, "Existing clients detected: %u", NimBLEDevice::getCreatedClientCount());
        if (advDevice)
        {
            ESP_LOGD(TAG, "Attempting reuse with peer: %s (RSSI during adv: %d)", advDevice->getAddress().toString().c_str(), advDevice->getRSSI());
        }
        /**
         *  Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if (pClient)
        {
            ESP_LOGD(TAG, "Found existing client for peer: %s; attempting fast reconnect (no svc refresh)", advDevice->getAddress().toString().c_str());
            if (!pClient->connect(advDevice, false))
            {
                ESP_LOGE(TAG, "Reconnect failed");
                return false;
            }
            ESP_LOGI(TAG, "Reconnected client");
        }
        else
        {
            /**
             *  We don't already have a client that knows this device,
             *  check for a client that is disconnected that we can use.
             */
            ESP_LOGD(TAG, "No existing client for peer; searching for disconnected client slot");
            pClient = NimBLEDevice::getDisconnectedClient();
            if (pClient)
            {
                ESP_LOGD(TAG, "Reusing a disconnected client instance");
            }
            else
            {
                ESP_LOGD(TAG, "No disconnected clients available; will create a new client");
            }
        }
    }

    /** No client to reuse? Create a new one. */
    if (!pClient)
    {
        if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS)
        {
            ESP_LOGE(TAG, "Max clients reached - no more connections available");
            return false;
        }

        pClient = NimBLEDevice::createClient();

        ESP_LOGI(TAG, "New client created");

        pClient->setClientCallbacks(&clientCallbacks, false);
        /**
         *  Set initial connection parameters:
         *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
         *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
         *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 150 * 10ms = 1500ms timeout
         */
        pClient->setConnectionParams(12, 12, 0, 150);
        ESP_LOGD(TAG, "Connection params set: min_itvl=12, max_itvl=12, latency=0, timeout=1500ms");

        /** Set how long we are willing to wait for the connection to complete (milliseconds), default is 30000. */
        pClient->setConnectTimeout(5 * 1000);
        ESP_LOGD(TAG, "Connect timeout set: %dms", 5 * 1000);

        ESP_LOGI(TAG, "Connecting to peer (new client): %s", advDevice->getAddress().toString().c_str());
        if (!pClient->connect(advDevice))
        {
            /** Created a client but failed to connect, don't need to keep it as it has no data */
            NimBLEDevice::deleteClient(pClient);
            ESP_LOGE(TAG, "Failed to connect, deleted client");
            return false;
        }
        ESP_LOGD(TAG, "Initial connect (new client) succeeded");
    }

    if (!pClient->isConnected())
    {
        ESP_LOGW(TAG, "Client instance exists but not connected; attempting normal connect");
        if (!pClient->connect(advDevice))
        {
            ESP_LOGE(TAG, "Failed to connect");
            return false;
        }
        ESP_LOGD(TAG, "Connect after existence check succeeded");
    }

    ESP_LOGI(TAG, "Connected to: %s RSSI: %d", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());

    /** Now we can read/write/subscribe the characteristics of the services we are interested in */
    NimBLERemoteService *pSvc = nullptr;
    NimBLERemoteCharacteristic *pChr = nullptr;
    NimBLERemoteDescriptor *pDsc = nullptr;

    pSvc = pClient->getService("DEAD");
    ESP_LOGD(TAG, "Lookup service 'DEAD' -> %s", pSvc ? "FOUND" : "NOT FOUND");
    if (pSvc)
    {
        pChr = pSvc->getCharacteristic("BEEF");
        ESP_LOGD(TAG, "Lookup char 'BEEF' in 'DEAD' -> %s", pChr ? "FOUND" : "NOT FOUND");
    }

    if (pChr)
    {
        if (pChr->canRead())
        {
            ESP_LOGD(TAG, "Reading 'BEEF'");
            ESP_LOGI(TAG, "%s Value: %s", pChr->getUUID().toString().c_str(), pChr->readValue().c_str());
        }

        if (pChr->canWrite())
        {
            ESP_LOGD(TAG, "Writing 'Tasty' to 'BEEF'");
            if (pChr->writeValue("Tasty"))
            {
                ESP_LOGI(TAG, "Wrote new value to: %s", pChr->getUUID().toString().c_str());
            }
            else
            {
                ESP_LOGE(TAG, "Write to 'BEEF' failed; disconnecting");
                pClient->disconnect();
                return false;
            }

            if (pChr->canRead())
            {
                ESP_LOGI(TAG, "The value of: %s is now: %s", pChr->getUUID().toString().c_str(), pChr->readValue().c_str());
            }
        }

        if (pChr->canNotify())
        {
            ESP_LOGD(TAG, "Subscribing to 'BEEF' notifications");
            if (!pChr->subscribe(true, notifyCB))
            {
                ESP_LOGE(TAG, "Subscribe to 'BEEF' notifications failed; disconnecting");
                pClient->disconnect();
                return false;
            }
            else
            {
                ESP_LOGD(TAG, "Subscribed to 'BEEF' notifications");
            }
        }
        else if (pChr->canIndicate())
        {
            /** Send false as first argument to subscribe to indications instead of notifications */
            ESP_LOGD(TAG, "Subscribing to 'BEEF' indications");
            if (!pChr->subscribe(false, notifyCB))
            {
                ESP_LOGE(TAG, "Subscribe to 'BEEF' indications failed; disconnecting");
                pClient->disconnect();
                return false;
            }
            else
            {
                ESP_LOGD(TAG, "Subscribed to 'BEEF' indications");
            }
        }
    }
    else
    {
        ESP_LOGW(TAG, "DEAD service not found.");
    }

    pSvc = pClient->getService(OSSM_SERVICE_UUID);
    ESP_LOGD(TAG, "Lookup service OSSM (%s) -> %s", String(OSSM_SERVICE_UUID).c_str(), pSvc ? "FOUND" : "NOT FOUND");
    if (pSvc)
    {
        pChr = pSvc->getCharacteristic(OSSM_CHARACTERISTIC_UUID);
        ESP_LOGD(TAG, "Lookup char OSSM (%s) -> %s", String(OSSM_CHARACTERISTIC_UUID).c_str(), pChr ? "FOUND" : "NOT FOUND");
        if (pChr)
        {
            if (pChr->canRead())
            {
                ESP_LOGD(TAG, "Reading OSSM characteristic before writes");
                ESP_LOGI(TAG, "%s Value: %s", pChr->getUUID().toString().c_str(), pChr->readValue().c_str());
            }

            pDsc = pChr->getDescriptor(NimBLEUUID("C01D"));
            ESP_LOGD(TAG, "Lookup descriptor C01D -> %s", pDsc ? "FOUND" : "NOT FOUND");
            if (pDsc)
            {
                ESP_LOGI(TAG, "Descriptor: %s  Value: %s", pDsc->getUUID().toString().c_str(), pDsc->readValue().c_str());
            }

            if (pChr->canWrite())
            {
                ESP_LOGD(TAG, "Writing 'go:strokeEngine' to OSSM characteristic");
                if (pChr->writeValue("go:strokeEngine"))
                {
                    ESP_LOGI(TAG, "Wrote new value to: %s", pChr->getUUID().toString().c_str());
                }
                else
                {
                    ESP_LOGE(TAG, "Write to OSSM characteristic failed; disconnecting");
                    pClient->disconnect();
                    return false;
                }

                if (pChr->canRead())
                {
                    ESP_LOGI(TAG, "The value of: %s is now: %s",
                             pChr->getUUID().toString().c_str(),
                             pChr->readValue().c_str());
                }
            }

            if (pChr->canNotify())
            {
                ESP_LOGD(TAG, "Subscribing to OSSM notifications");
                if (!pChr->subscribe(true, notifyCB))
                {
                    ESP_LOGE(TAG, "Subscribe to OSSM notifications failed; disconnecting");
                    pClient->disconnect();
                    return false;
                }
                else
                {
                    ESP_LOGD(TAG, "Subscribed to OSSM notifications");
                }
            }
            else if (pChr->canIndicate())
            {
                /** Send false as first argument to subscribe to indications instead of notifications */
                ESP_LOGD(TAG, "Subscribing to OSSM indications");
                if (!pChr->subscribe(false, notifyCB))
                {
                    ESP_LOGE(TAG, "Subscribe to OSSM indications failed; disconnecting");
                    pClient->disconnect();
                    return false;
                }
                else
                {
                    ESP_LOGD(TAG, "Subscribed to OSSM indications");
                }
            }
        }
    }
    else
    {
        ESP_LOGW(TAG, "BAAD service not found.");
    }

    ESP_LOGI(TAG, "Done with this device!");
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

    xTaskCreatePinnedToCore(
        [](void *pvParameter)
        {
            vTaskDelay(1000);
            while (true)
            {
                vTaskDelay(100);

                if (doConnect)
                {
                    doConnect = false;
                    /** Found a device we want to connect to, do it now */
                    if (connectToServer())
                    {
                        ESP_LOGI(TAG, "Success! we should now be getting notifications, scanning for more!");
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Failed to connect, starting scan");
                        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
                    }
                }

                // if we're not connected continue.
                const NimBLEAdvertisedDevice *currentAdv = advDevice;
                if (currentAdv == nullptr)
                {
                    ESP_LOGV(TAG, "No advertised device available yet");
                    continue;
                }

                NimBLEClient *client = NimBLEDevice::getClientByPeerAddress(currentAdv->getAddress());
                if (client == nullptr)
                {
                    ESP_LOGV(TAG, "No client instance for peer: %s", currentAdv->getAddress().toString().c_str());
                    continue;
                }

                if (!client->isConnected())
                {
                    ESP_LOGV(TAG, "Client exists but not connected to: %s", currentAdv->getAddress().toString().c_str());
                    continue;
                }

                // first check for changes in lastSettings
                if (lastSettings.speed != settings.speed)
                {
                    sendCommand("set:speed:" + String(settings.speed, 0));
                    lastSettings.speed = settings.speed;
                    ESP_LOGI(TAG, "Speed changed, sending command: set:speed:%d", settings.speed);
                }

                if (lastSettings.stroke != settings.stroke)
                {
                    sendCommand("set:stroke:" + String(settings.stroke, 0));
                    lastSettings.stroke = settings.stroke;
                    ESP_LOGI(TAG, "Stroke changed, sending command: set:stroke:%d", settings.stroke);
                }

                if (lastSettings.sensation != settings.sensation)
                {
                    sendCommand("set:sensation:" + String(settings.sensation, 0));
                    lastSettings.sensation = settings.sensation;
                    ESP_LOGI(TAG, "Sensation changed, sending command: set:sensation:%d", settings.sensation);
                }

                if (lastSettings.depth != settings.depth)
                {
                    sendCommand("set:depth:" + String(settings.depth, 0));
                    lastSettings.depth = settings.depth;
                    ESP_LOGI(TAG, "Depth changed, sending command: set:depth:%d", settings.depth);
                }

                if (lastSettings.pattern != settings.pattern)
                {
                    sendCommand("set:pattern:" + String(static_cast<uint8_t>(settings.pattern), 0));
                    lastSettings.pattern = settings.pattern;
                    ESP_LOGI(TAG, "Pattern changed, sending command: set:pattern:%d", static_cast<uint8_t>(settings.pattern));
                }

                // if there's nothing in the queue, continue.
                if (commandQueue.empty())
                {
                    continue;
                }

                // if there's something in the queue, send it.
                String command = commandQueue.front();
                commandQueue.pop();
                ESP_LOGI(TAG, "Sending command: %s", command.c_str());
                NimBLERemoteService *svc = client->getService(OSSM_SERVICE_UUID);
                if (svc == nullptr)
                {
                    ESP_LOGW(TAG, "OSSM service not found on peer; dropping command");
                    continue;
                }

                NimBLERemoteCharacteristic *pChr = svc->getCharacteristic(OSSM_CHARACTERISTIC_UUID);
                if (pChr == nullptr)
                {
                    ESP_LOGW(TAG, "OSSM characteristic not found on peer; dropping command");
                    continue;
                }

                if (!pChr->canWrite())
                {
                    ESP_LOGW(TAG, "OSSM characteristic not writable; dropping command");
                    continue;
                }

                if (pChr->writeValue(command))
                {
                    ESP_LOGI(TAG, "Command sent: %s", command.c_str());
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to write command to OSSM characteristic");
                }
            }
        },
        "BLE Task",
        10 * configMINIMAL_STACK_SIZE,
        NULL,
        1,
        NULL,
        1);
}
