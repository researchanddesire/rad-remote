#pragma once

#ifndef OSSM_DEVICE_H
#define OSSM_DEVICE_H

#include "../../device.hpp"
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
    }

    const char *getName() override { return "OSSM"; }
    NimBLEUUID getServiceUUID() override { return NimBLEUUID(OSSM_SERVICE_ID); }

    SettingPercents settings;

    void onConnect() override
    {
        readJsonValues("state", [this](const JsonObject &state)
                       {
                        this->settings.speed = state["speed"].as<float>();
                        this->settings.stroke = state["stroke"].as<float>();
                        this->settings.sensation = state["sensation"].as<float>();
                        this->settings.depth = state["depth"].as<float>(); });

        send("speedKnobLimit", "false");
        send("command", "go:strokeEngine");
        send("command", "go:strokeEngine");
    }

    bool setSpeed(int speed)
    {
        return send("command", std::string("set:speed:") + std::to_string(speed));
    }

    bool setDepth(int depth)
    {
        return send("command", std::string("set:depth:") + std::to_string(depth));
    }

    bool setStroke(int stroke)
    {
        return send("command", std::string("set:stroke:") + std::to_string(stroke));
    }

    bool setSensation(int sensation)
    {
        return send("command", std::string("set:sensation:") + std::to_string(sensation));
    }

    bool setPattern(int pattern)
    {
        return send("command", std::string("set:pattern:") + std::to_string(pattern));
    }
};

#endif // OSSM_DEVICE_H
