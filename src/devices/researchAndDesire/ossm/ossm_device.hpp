#pragma once

#ifndef OSSM_DEVICE_H
#define OSSM_DEVICE_H

#include "../../device.h"
#include <components/TextButton.h>
#include <components/LinearRailGraph.h>
#include <components/EncoderDial.h>
#include <ArduinoJson.h>

#define OSSM_CHARACTERISTIC_UUID_COMMAND "522B443A-4F53-534D-0002-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID_STATE "522b443a-4f53-534d-1000-420badbabe69"
#define OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT "522B443A-4F53-534D-0010-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID_PATTERNS "522b443a-4f53-534d-2000-420badbabe69"

class OSSM : public Device
{
public:
    SettingPercents settings;
    int rightFocusedIndex = 0;
    int leftFocusedIndex = 0;

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

    void drawControls() override
    {

        leftEncoder.setBoundaries(0, 100);
        leftEncoder.setAcceleration(50);
        leftEncoder.setEncoderValue(settings.speed);

        rightEncoder.setBoundaries(0, 100);
        rightEncoder.setAcceleration(50);
        rightEncoder.setEncoderValue(settings.stroke);

        // Top bumpers
        draw<TextButton>("<-", pins::BTN_L_SHOULDER, 0, 0);
        draw<TextButton>("->", pins::BTN_R_SHOULDER, DISPLAY_WIDTH - 60, 0);

        // Bottom bumpers
        draw<TextButton>("Home", pins::BTN_UNDER_L, 0, DISPLAY_HEIGHT - 25);
        draw<TextButton>("Patterns", pins::BTN_UNDER_R, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT - 25);

        draw<TextButton>("STOP", pins::BTN_UNDER_C, DISPLAY_WIDTH / 2 - 60, DISPLAY_HEIGHT - 25, 120);

        draw<LinearRailGraph>(&this->settings.stroke, &this->settings.depth, -1, Display::PageHeight - 40, Display::WIDTH);

        // Create a left encoder dial with Speed parameter
        std::map<String, float *> leftParams = {
            {"Speed", &this->settings.speed}};
        draw<EncoderDial>(EncoderDial::Props{
            .encoder = &leftEncoder,
            .parameters = leftParams,
            .focusedIndex = &this->leftFocusedIndex,
            .x = 0,
            .y = (int16_t)(Display::PageY + 10)});

        // Create a right encoder dial with all parameters
        std::map<String, float *> rightParams = {
            {"Stroke", &this->settings.stroke},
            {"Depth", &this->settings.depth},
            {"Sens.", &this->settings.sensation}};
        draw<EncoderDial>(EncoderDial::Props{
            .encoder = &rightEncoder,
            .parameters = rightParams,
            .focusedIndex = &this->rightFocusedIndex,
            .x = (int16_t)(DISPLAY_WIDTH - 90),
            .y = (int16_t)(Display::PageY + 10)});
    }

    void onConnect() override
    {
        // when we connect, pull the current state from the device
        readJson<JsonObject>("state", [this](const JsonObject &state)
                             {
                        this->settings.speed = state["speed"].as<float>();
                        this->settings.stroke = state["stroke"].as<float>();
                        this->settings.sensation = state["sensation"].as<float>();
                        this->settings.depth = state["depth"].as<float>(); 
                        this->settings.pattern = static_cast<StrokePatterns>(state["pattern"].as<int>()); 
                    
                    ESP_LOGI(TAG, "UPDATED SETTINGS: Speed: %d, Stroke: %d, Sensation: %d, Depth: %d, Pattern: %d", this->settings.speed, this->settings.stroke, this->settings.sensation, this->settings.depth, this->settings.pattern); });

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
        vTaskDelay(pdMS_TO_TICKS(100));
        send("speedKnobLimit", "false");
        vTaskDelay(pdMS_TO_TICKS(100));
        send("command", "go:strokeEngine");
        vTaskDelay(pdMS_TO_TICKS(100));
        // TODO: A bug on AJ's dev unit requires two "go:strokeEngine" commands.
        send("command", "go:strokeEngine");
        vTaskDelay(pdMS_TO_TICKS(100));

        isConnected = true;

// SPAM TEST
#ifdef SPAM_OSSM_TEST
        int i = 0;
        bool goingUp = true;
        while (true)
        {
            setSpeed(i);
            leftEncoder.setEncoderValue(i);
            i += goingUp ? 1 : -1;
            if (i >= 100)
            {
                goingUp = false;
            }
            if (i <= 0)
            {
                goingUp = true;
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
#endif
    }

    void onStop() override
    {
        send("command", "go:idle");
    }

    void onDeviceMenuItemSelected(int index) override
    {
        setPattern(index);
    }

    // Helper functions.
    bool setSpeed(int speed)
    {
        if (speed == settings.speed)
        {
            return true;
        }
        settings.speed = speed;
        speed = constrain(speed, 0, 100);
        return send("command", std::string("set:speed:") + std::to_string(speed));
    }

    float getDepth()
    {
        return constrain(settings.depth, 0.0f, 100.0f);
    }

    bool setDepth(int depth)
    {
        if (depth == settings.depth)
        {
            return true;
        }
        settings.depth = depth;
        depth = constrain(depth, 0, 100);
        return send("command", std::string("set:depth:") + std::to_string(depth));
    }

    float getStroke()
    {
        return constrain(settings.stroke, 0.0f, 100.0f);
    }

    bool setStroke(int stroke)
    {
        if (stroke == settings.stroke)
        {
            return true;
        }
        settings.stroke = stroke;
        stroke = constrain(stroke, 0, 100);
        return send("command", std::string("set:stroke:") + std::to_string(stroke));
    }

    bool setSensation(int sensation)
    {
        if (sensation == settings.sensation)
        {
            return true;
        }
        settings.sensation = sensation;
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
        if (pattern == static_cast<int>(settings.pattern))
        {
            return true;
        }
        settings.pattern = static_cast<StrokePatterns>(pattern);
        pattern = pattern % menu.size();
        int patternIdx = menu[pattern].metaIndex;
        return send("command", std::string("set:pattern:") + std::to_string(patternIdx));
    }

    void onLeftBumperClick() override
    {
        rightFocusedIndex = (rightFocusedIndex + 2) % 3; // Safe decrement and wrap: 0->2, 1->0, 2->1
        if (rightFocusedIndex == 0)
        {
            rightEncoder.setEncoderValue(settings.depth);
        }
        else if (rightFocusedIndex == 1)
        {
            rightEncoder.setEncoderValue(settings.sensation);
        }
        else if (rightFocusedIndex == 2)
        {
            rightEncoder.setEncoderValue(settings.stroke);
        }
    }

    void onRightBumperClick() override
    {
        rightFocusedIndex = (rightFocusedIndex + 1) % 3; // Safe increment and wrap: 2->0
        if (rightFocusedIndex == 0)
        {
            rightEncoder.setEncoderValue(settings.depth);
        }
        else if (rightFocusedIndex == 1)
        {
            rightEncoder.setEncoderValue(settings.sensation);
        }
        else if (rightFocusedIndex == 2)
        {
            rightEncoder.setEncoderValue(settings.stroke);
        }
    }

    void onRightEncoderChange(int value) override
    {
        if (rightFocusedIndex == 0)
        {
            setDepth(value);
        }
        else if (rightFocusedIndex == 1)
        {
            setSensation(value);
        }
        else if (rightFocusedIndex == 2)
        {
            setStroke(value);
        }
    }

    void onLeftEncoderChange(int value) override
    {
        setSpeed(value);
    }
};

#endif // OSSM_DEVICE_H
