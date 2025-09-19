#pragma once

#ifndef OSSM_DEVICE_H
#define OSSM_DEVICE_H

#include "../../device.h"
#include <ArduinoJson.h>

#define OSSM_CHARACTERISTIC_UUID_COMMAND "522B443A-4F53-534D-0002-420BADBABE69"
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
            {"command", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_COMMAND)}},
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
        // when we connect, pull the current state from the device
        readJson<JsonObject>("state", [this](const JsonObject &state)
                             {
                        this->settings.speed = state["speed"].as<float>();
                        this->settings.stroke = state["stroke"].as<float>();
                        this->settings.sensation = state["sensation"].as<float>();
                        this->settings.depth = state["depth"].as<float>(); 
                        this->settings.pattern = static_cast<StrokePatterns>(state["pattern"].as<int>()); });

        // And then we pull the patterns from the device
        readJson<JsonArray>("patterns", [this](const JsonArray &patterns)
                            {
                                // clear the patterns vector
                                this->menu.clear();

                                for (JsonVariant v : patterns)
                                {

                                    auto icon = researchAndDesireWaves;
                                    std::string name = v["name"].as<const char*>();
                                    std::string lowerName = name;
                                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                                    if (lowerName.find("simple") != std::string::npos)
                                    {
                                        icon = researchAndDesireWaves;
                                    }
                                    else if (lowerName.find("teasing") != std::string::npos)
                                    {
                                        icon = researchAndDesireHeart;
                                    }
                                    else if (lowerName.find("robo") != std::string::npos)
                                    {
                                        icon = researchAndDesireTerminal;
                                    }
                                    else if (lowerName.find("stop") != std::string::npos)
                                    {
                                        icon = researchAndDesireHourglass03;
                                    }
                                    else if (lowerName.find("insist") != std::string::npos)
                                    {
                                        icon = researchAndDesireItalic02;
                                    }
                                    else if (lowerName.find("deeper") != std::string::npos)
                                    {
                                        icon = researchAndDesireArrowsRight;
                                    }
                                    else if (lowerName.find("half") != std::string::npos)
                                    {
                                        icon = researchAndDesireFaceWink;
                                    }

                                    ESP_LOGI(TAG, "Pattern: %s, %d", name.c_str(), v["idx"].as<int>());
                                    this->menu.push_back(MenuItem{
                                        MenuItemE::DEVICE_MENU_ITEM,
                                        name,
                                        icon,
                                        .metaIndex = v["idx"].as<int>()});
                                } });

        // finally, we set inital preferences and go to stroke engine mode
        send("speedKnobLimit", "false");
        send("command", "go:strokeEngine");
        // TODO: A bug on AJ's dev unit requires two "go:strokeEngine" commands.
        send("command", "go:strokeEngine");
    }

    void drawControls() override
    {
        ESP_LOGI(TAG, "Drawing controls for OSSM");
        // small delay to ensure tasks are finished
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            tft.fillScreen(Colors::black);
            tft.setTextSize(1);
            tft.setTextColor(Colors::white);
            tft.setCursor(0, Display::PageY);
            tft.println("OSSM CONTROLS");
            xSemaphoreGive(displayMutex);
        }
    }

    void drawDeviceMenu() override
    {

        activeMenu = menu.data();
        activeMenuCount = menu.size();

        drawMenu();
    }

    void onDeviceMenuItemSelected(int index) override
    {
        setPattern(index);
    }

private:
    // Helper functions.
    bool setSpeed(int speed)
    {
        speed = constrain(speed, 0, 100);
        return send("command", std::string("set:speed:") + std::to_string(speed));
    }

    bool setDepth(int depth)
    {
        depth = constrain(depth, 0, 100);
        return send("command", std::string("set:depth:") + std::to_string(depth));
    }

    bool setStroke(int stroke)
    {
        stroke = constrain(stroke, 0, 100);
        return send("command", std::string("set:stroke:") + std::to_string(stroke));
    }

    bool setSensation(int sensation)
    {
        sensation = constrain(sensation, 0, 100);
        return send("command", std::string("set:sensation:") + std::to_string(sensation));
    }

    bool setPattern(int pattern)
    {
        if (menu.empty())
        {
            ESP_LOGW(TAG, "setPattern called but menu is empty");
            return false;
        }
        pattern = pattern % menu.size();
        int patternIdx = menu[pattern].metaIndex;
        return send("command", std::string("set:pattern:") + std::to_string(patternIdx));
    }
};

#endif // OSSM_DEVICE_H
