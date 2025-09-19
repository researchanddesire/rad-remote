#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include "devices/serviceUUIDs.h"
#include "state/remote.h"
#include <unordered_map>

#define TAG "DEVICE"
#define OSSM_CHARACTERISTIC_UUID "522B443A-4F53-534D-0002-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT "522B443A-4F53-534D-0010-420BADBABE69"

// Forward declaration to avoid heavy include and ensure pointer type is known

struct DeviceCharacteristics
{
    NimBLEUUID uuid;
    String key;
    String value;
    NimBLERemoteCharacteristic *pCharacteristic = nullptr;
};

class Device : public NimBLEClientCallbacks
{
public:
    const NimBLEAdvertisedDevice *advertisedDevice;
    NimBLEClient *pClient;
    NimBLERemoteService *pService;

    JsonObject settings;
    JsonDocument settingsDoc;

    std::unordered_map<std::string, DeviceCharacteristics> characteristics;

    // Constructor with settings document size parameter
    explicit Device(const NimBLEAdvertisedDevice *advertisedDevice) : settingsDoc(), advertisedDevice(advertisedDevice)
    {
        startConnectionTask();
    }

    // Virtual destructor for proper cleanup
    virtual ~Device() = default;

    // Virtual methods that child classes can optionally override
    virtual void onRightBumperClick() {}
    virtual void onLeftBumperClick() {}
    virtual void onRightButtonClick() {}
    virtual void onLeftButtonClick() {}
    virtual void onExit() {}
    virtual void onConnect() {}
    virtual void onDisconnect() {}

    virtual void onRightEncoderChange() {}
    virtual void onLeftEncoderChange() {}

    virtual void drawControls() {}
    virtual void drawMenu() {}

    virtual void pullValue() {}
    virtual void pushValue() {}

    virtual NimBLEUUID getServiceUUID() = 0;
    virtual const char *getName() = 0;

protected:
    TaskHandle_t connectionTaskHandle;
    static void connectionTask(void *pvParameter)
    {
        Device *device = (Device *)pvParameter;
        const NimBLEAdvertisedDevice *advDevice = device->advertisedDevice;
        bool connected = false;
        while (true)
        {
            ESP_LOGI(TAG, "Connection task running");

            vTaskDelay(1000);
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
                        ESP_LOGE(TAG, "Reconnect failed, deleting client and trying again.");
                        NimBLEDevice::deleteClient(pClient);
                        connected = false;
                        continue;
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

            vTaskDelay(1);

            /** No client to reuse? Create a new one. */
            if (!pClient)
            {
                ESP_LOGD(TAG, "No client to reuse, creating new client");
                if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS)
                {
                    ESP_LOGE(TAG, "Max clients reached - no more connections available");
                    connected = false;

                    // TODO: Schedule for deleting and throw error.
                    break;
                }

                ESP_LOGD(TAG, "Creating new client");
                pClient = NimBLEDevice::createClient();

                ESP_LOGI(TAG, "New client created");

                pClient->setClientCallbacks(device, false);
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
                    connected = false;
                    break;
                }
                ESP_LOGD(TAG, "Initial connect (new client) succeeded");
            }

            vTaskDelay(1);

            ESP_LOGD(TAG, "Checking if client is connected");

            if (!pClient->isConnected())
            {
                ESP_LOGW(TAG, "Client instance exists but not connected; attempting normal connect");
                if (!pClient->connect(advDevice))
                {
                    ESP_LOGE(TAG, "Failed to connect");
                    connected = false;
                    break;
                }
                ESP_LOGD(TAG, "Connect after existence check succeeded");
            }
            vTaskDelay(1);

            ESP_LOGI(TAG, "Connected to: %s RSSI: %d", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());

            device->pClient = pClient;
            device->pService = pClient->getService(device->getServiceUUID());
            if (device->pService == nullptr)
            {
                ESP_LOGE(TAG, "Service not found");
                stateMachine->process_event(conncted_error_event());
                break;
            }

            // for each characteristic in the device characteristics vector, get the characteristic and save it.
            for (auto &characteristic : device->characteristics)
            {
                vTaskDelay(1);
                ESP_LOGD(TAG, "Getting characteristic: %s", characteristic.first.c_str());
                characteristic.second.pCharacteristic = device->pService->getCharacteristic(characteristic.second.uuid);
            }

            vTaskDelay(1);
            // run the user defined "on connect" method.
            device->onConnect();

            // // ESP_LOGD(TAG, "Lookup service OSSM (%s) -> %s", device->serviceUUID.c_str(), pSvc ? "FOUND" : "NOT FOUND");
            // if (pSvc)
            // {
            //     pChr = pSvc->getCharacteristic(OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT);
            //     ESP_LOGD(TAG, "Lookup char OSSM (%s) -> %s", String(OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT).c_str(), pChr ? "FOUND" : "NOT FOUND");
            //     if (pChr && pChr->canWrite())
            //     {
            //         pChr->writeValue("false");
            //     }

            //     pChr = pSvc->getCharacteristic(OSSM_CHARACTERISTIC_UUID);
            //     ESP_LOGD(TAG, "Lookup char OSSM (%s) -> %s", String(OSSM_CHARACTERISTIC_UUID).c_str(), pChr ? "FOUND" : "NOT FOUND");
            //     if (pChr && pChr->canWrite())
            //     {
            //         pChr->writeValue("go:strokeEngine");
            //         pChr->writeValue("go:strokeEngine");
            //     }
            // }
            // else
            // {
            //     ESP_LOGW(TAG, "BAAD service not found.");
            // }

            ESP_LOGI(TAG, "Done with this device!");

            break;
        }
        vTaskDelete(NULL);
    }

    void onConnect(NimBLEClient *pClient) override
    {
        ESP_LOGD(TAG, "Connected to %s", getName());
        stateMachine->process_event(connected_event());
    }

    void onDisconnect(NimBLEClient *pClient, int reason) override
    {
        ESP_LOGD(TAG, "Disconnected from %s", getName());
        stateMachine->process_event(disconnected_event());
        onDisconnect();
    }

    void send(const std::string &command, const std::string &value)
    {
        auto it = characteristics.find(command);
        if (it == characteristics.end())
        {
            ESP_LOGW(TAG, "Characteristic '%s' not found for device '%s'", command.c_str(), getName());
            return;
        }

        auto *pChr = it->second.pCharacteristic;
        if (!pChr)
        {
            ESP_LOGW(TAG, "Characteristic '%s' exists but pCharacteristic is nullptr for device '%s'", command.c_str(), getName());
            return;
        }

        if (!pChr->canWrite())
        {
            ESP_LOGW(TAG, "Characteristic '%s' for device '%s' is not writable", command.c_str(), getName());
            return;
        }

        ESP_LOGI(TAG, "Writing value '%s' to characteristic '%s' on device '%s'", value.c_str(), command.c_str(), getName());
        pChr->writeValue(value);
    }

private:
    void startConnectionTask()
    {
        auto stackSize = 20 * configMINIMAL_STACK_SIZE;
        xTaskCreatePinnedToCore(Device::connectionTask, "connectionTask", stackSize, this, 1, &connectionTaskHandle, 0);
    }
};

#endif // DEVICE_H
