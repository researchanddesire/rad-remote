#pragma once

#ifndef OSSM_DEVICE_H
#define OSSM_DEVICE_H

#include "../../device.hpp"
#include <ArduinoJson.h>

class OSSM : public Device
{
public:
    explicit OSSM(const NimBLEAdvertisedDevice *advertisedDevice)
        : Device(advertisedDevice)
    {

        characteristics = {
            {"command", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID), "Command", "Command"}},
            {"speedKnobLimit", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT), "Speed Knob Limit", "Speed Knob Limit"}}};
    }

    NimBLEUUID getServiceUUID() override { return NimBLEUUID(OSSM_SERVICE_ID); }
    const char *getName() override { return "OSSM"; }

    void onConnect() override
    {
        send("speedKnobLimit", "false");
        send("command", "go:strokeEngine");
        send("command", "go:strokeEngine");
    }

    void onDisconnect() override
    {
        ESP_LOGI(TAG, "Disconnected from OSSM");
    }
};

#endif // OSSM_DEVICE_H
