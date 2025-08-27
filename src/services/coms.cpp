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

static const char *TAG = "COMS";

static const NimBLEAdvertisedDevice *advDevice;
static bool doConnect = false;
static uint32_t scanTimeMs = 1000; /** scan time in milliseconds, 0 = scan forever */

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks
{
    void onConnect(NimBLEClient *pClient) override { ESP_LOGI(TAG, "Connected"); }

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
        ESP_LOGI(TAG, "Advertised Device found: %s", advertisedDevice->toString().c_str());
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
        ESP_LOGI(TAG, "Scan Ended, reason: %d, device count: %d; Restarting scan", reason, results.getCount());
        vTaskDelay(2000);
        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
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
    ESP_LOGI(TAG, "%s", str.c_str());
}

/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer()
{
    NimBLEClient *pClient = nullptr;

    /** Check if we have a client we should reuse first **/
    if (NimBLEDevice::getCreatedClientCount())
    {
        /**
         *  Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if (pClient)
        {
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
            pClient = NimBLEDevice::getDisconnectedClient();
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

        /** Set how long we are willing to wait for the connection to complete (milliseconds), default is 30000. */
        pClient->setConnectTimeout(5 * 1000);

        if (!pClient->connect(advDevice))
        {
            /** Created a client but failed to connect, don't need to keep it as it has no data */
            NimBLEDevice::deleteClient(pClient);
            ESP_LOGE(TAG, "Failed to connect, deleted client");
            return false;
        }
    }

    if (!pClient->isConnected())
    {
        if (!pClient->connect(advDevice))
        {
            ESP_LOGE(TAG, "Failed to connect");
            return false;
        }
    }

    ESP_LOGI(TAG, "Connected to: %s RSSI: %d", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());

    /** Now we can read/write/subscribe the characteristics of the services we are interested in */
    NimBLERemoteService *pSvc = nullptr;
    NimBLERemoteCharacteristic *pChr = nullptr;
    NimBLERemoteDescriptor *pDsc = nullptr;

    pSvc = pClient->getService("DEAD");
    if (pSvc)
    {
        pChr = pSvc->getCharacteristic("BEEF");
    }

    if (pChr)
    {
        if (pChr->canRead())
        {
            ESP_LOGI(TAG, "%s Value: %s", pChr->getUUID().toString().c_str(), pChr->readValue().c_str());
        }

        if (pChr->canWrite())
        {
            if (pChr->writeValue("Tasty"))
            {
                ESP_LOGI(TAG, "Wrote new value to: %s", pChr->getUUID().toString().c_str());
            }
            else
            {
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
            if (!pChr->subscribe(true, notifyCB))
            {
                pClient->disconnect();
                return false;
            }
        }
        else if (pChr->canIndicate())
        {
            /** Send false as first argument to subscribe to indications instead of notifications */
            if (!pChr->subscribe(false, notifyCB))
            {
                pClient->disconnect();
                return false;
            }
        }
    }
    else
    {
        ESP_LOGW(TAG, "DEAD service not found.");
    }

    pSvc = pClient->getService(OSSM_SERVICE_UUID);
    if (pSvc)
    {
        pChr = pSvc->getCharacteristic(OSSM_CHARACTERISTIC_UUID);
        if (pChr)
        {
            if (pChr->canRead())
            {
                ESP_LOGI(TAG, "%s Value: %s", pChr->getUUID().toString().c_str(), pChr->readValue().c_str());
            }

            pDsc = pChr->getDescriptor(NimBLEUUID("C01D"));
            if (pDsc)
            {
                ESP_LOGI(TAG, "Descriptor: %s  Value: %s", pDsc->getUUID().toString().c_str(), pDsc->readValue().c_str());
            }

            if (pChr->canWrite())
            {
                if (pChr->writeValue("go:strokeEngine"))
                {
                    ESP_LOGI(TAG, "Wrote new value to: %s", pChr->getUUID().toString().c_str());
                }
                else
                {
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
                if (!pChr->subscribe(true, notifyCB))
                {
                    pClient->disconnect();
                    return false;
                }
            }
            else if (pChr->canIndicate())
            {
                /** Send false as first argument to subscribe to indications instead of notifications */
                if (!pChr->subscribe(false, notifyCB))
                {
                    pClient->disconnect();
                    return false;
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
    pScan->setInterval(100);
    pScan->setWindow(100);

    /**
     * Active scan will gather scan response data from advertisers
     *  but will use more energy from both devices
     */
    pScan->setActiveScan(true);

    /** Start scanning for advertisers */
    pScan->start(scanTimeMs);
    ESP_LOGI(TAG, "Scanning for peripherals");

    xTaskCreate(
        [](void *pvParameter)
        {
            while (true)
            {
                vTaskDelay(10);

                if (!doConnect)
                {
                    continue;
                }

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
        },
        "BLE Task",
        10000,
        NULL,
        1,
        NULL);
}
