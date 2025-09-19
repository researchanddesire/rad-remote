#pragma once

#ifndef OSSM_DEVICE_H
#define OSSM_DEVICE_H

#include "../../device.hpp"
#include "../../utils.hpp"
#include <ArduinoJson.h>

#define OSSM_CHARACTERISTIC_UUID "522B443A-4F53-534D-0002-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID_STATE "522b443a-4f53-534d-1000-420badbabe69"
#define OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT "522B443A-4F53-534D-0010-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID_PATTERNS "522b443a-4f53-534d-2000-420badbabe69"

class OSSM : public Device
{
public:
    explicit OSSM(const NimBLEAdvertisedDevice *advertisedDevice)
        : Device(advertisedDevice)
    {

        characteristics = {
            {"command", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID)}},
            {"speedKnobLimit", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT)}},
            {"patterns", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_PATTERNS)}},
            {"state", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_STATE)}},
        };

        characteristics["state"].decode = [](const std::string &input) -> std::string
        { return passthroughString(input.c_str()).c_str(); };
    }

    const char *getName() override { return "OSSM"; }
    NimBLEUUID getServiceUUID() override { return NimBLEUUID(OSSM_SERVICE_ID); }

    SettingPercents settings = {
        .speed = 0,
        .stroke = 0,
        .sensation = 0,
        .depth = 0,
        .pattern = StrokePatterns::SimpleStroke,
        .speedKnob = 0};

    void onConnect() override
    {
        readChar<std::string>("state");

        send("speedKnobLimit", "false");
        send("command", "go:strokeEngine");
        send("command", "go:strokeEngine");
        send("command", "set:speed:100");
        send("command", "set:depth:10");
        send("command", "set:stroke:10");
        send("command", "set:sensation:10");
        send("command", "set:pattern:2");
    }

    void onDisconnect() override
    {
        ESP_LOGI(TAG, "Disconnected from OSSM");
    }
};

#endif // OSSM_DEVICE_H
