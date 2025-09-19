#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include "devices/serviceUUIDs.h"
#include <unordered_map>
#include <functional>
#include <structs/Menus.h>

#define TAG "DEVICE"

// Forward declaration to avoid heavy include and ensure pointer type is known

class Device;

extern Device *device;

struct DeviceCharacteristics
{
    NimBLEUUID uuid;
    NimBLERemoteCharacteristic *pCharacteristic = nullptr;

    // Function pointers for custom encode/decode
    std::function<std::string(const std::string &)> encode = nullptr;
};

class Device : public NimBLEClientCallbacks
{
public:
    const NimBLEAdvertisedDevice *advertisedDevice;
    NimBLEClient *pClient;
    NimBLERemoteService *pService;

    std::vector<MenuItem> menu;

    std::unordered_map<std::string, DeviceCharacteristics> characteristics;

    // Constructor with settings document size parameter
    explicit Device(const NimBLEAdvertisedDevice *advertisedDevice);

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
    virtual void onDeviceMenuItemSelected(int index) {}

    virtual void onRightEncoderChange() {}
    virtual void onLeftEncoderChange() {}

    virtual void drawControls() {}
    virtual void drawDeviceMenu() {}

    virtual void pullValue() {}
    virtual void pushValue() {}

    virtual NimBLEUUID getServiceUUID() = 0;
    virtual const char *getName() = 0;

protected:
    TaskHandle_t connectionTaskHandle;
    static void connectionTask(void *pvParameter);

    void onConnect(NimBLEClient *pClient) override;

    void onDisconnect(NimBLEClient *pClient, int reason) override;

    bool send(const std::string &command, const std::string &value);

    std::string readString(const std::string &command);

    // Helper method to safely read JSON values
    template <typename T>
    T readJsonValue(const std::string &command, const char *key, T defaultValue)
    {
        auto value = readString(command);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, value.c_str());

        if (error)
        {
            ESP_LOGE("DEVICE", "JSON parse failed: %s", error.c_str());
            return defaultValue;
        }

        if (!doc.containsKey(key))
        {
            ESP_LOGW("DEVICE", "JSON key '%s' not found", key);
            return defaultValue;
        }

        return doc[key].as<T>();
    }

    template <typename T = JsonObject>
    bool readJson(const std::string &command, std::function<void(const T &)> callback)
    {
        auto value = readString(command);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, value.c_str());

        if (error)
        {
            ESP_LOGE("DEVICE", "JSON parse failed: %s", error.c_str());
            return false;
        }

        callback(doc.as<T>());
        return true;
    }

    // Legacy method - now returns a copy of the JSON as string for safety
    std::string readJsonString(const std::string &command);

private:
    void startConnectionTask();
};

#endif // DEVICE_H
