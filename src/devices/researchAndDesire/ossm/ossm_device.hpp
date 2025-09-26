#pragma once

#ifndef OSSM_DEVICE_H
#define OSSM_DEVICE_H

#include <ArduinoJson.h>
#include <components/DynamicText.h>
#include <components/EncoderDial.h>
#include <components/LinearRailGraph.h>
#include <components/TextButton.h>

#include "../../device.h"

#define OSSM_CHARACTERISTIC_UUID_COMMAND "522B443A-4F53-534D-1000-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT \
    "522B443A-4F53-534D-1010-420BADBABE69"

#define OSSM_CHARACTERISTIC_UUID_STATE "522b443a-4f53-534d-2000-420badbabe69"
#define OSSM_CHARACTERISTIC_UUID_PATTERNS "522b443a-4f53-534d-3000-420badbabe69"
#define OSSM_CHARACTERISTIC_UUID_PATTERN_DESCRIPTION \
    "522b443a-4f53-534d-3010-420badbabe69"

class OSSM : public Device {
  public:
    SettingPercents settings;
    int rightFocusedIndex = 0;
    int leftFocusedIndex = 0;
    std::string patternName = DEFAULT_OSSM_PATTERN_NAME;
    bool isFirstConnect = true;

    explicit OSSM(const NimBLEAdvertisedDevice *advertisedDevice)
        : Device(advertisedDevice) {
        characteristics = {
            {"command", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_COMMAND)}},
            {"speedKnobLimit",
             {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_SET_SPEED_KNOB_LIMIT)}},
            {"patterns", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_PATTERNS)}},
            {"patternDescription",
             {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_PATTERN_DESCRIPTION)}},
            {"state", {NimBLEUUID(OSSM_CHARACTERISTIC_UUID_STATE)}},
        };
    }

    const char *getName() override { return "OSSM"; }
    NimBLEUUID getServiceUUID() override { return NimBLEUUID(OSSM_SERVICE_ID); }

    void drawControls() override {
        leftEncoder.setBoundaries(0, 100);
        leftEncoder.setAcceleration(50);
        leftEncoder.setEncoderValue(settings.speed);

        rightEncoder.setBoundaries(0, 100);
        rightEncoder.setAcceleration(50);

        syncRightEncoder();

        // Top bumpers - positioned with margin to prevent border cutoff
        draw<TextButton>("<-", pins::BTN_L_SHOULDER, 5, 0);
        draw<TextButton>("->", pins::BTN_R_SHOULDER, DISPLAY_WIDTH - 75, 0);

        // Bottom bumpers - positioned with margin to prevent border cutoff
        draw<TextButton>("Home", pins::BTN_UNDER_L, 5, Display::HEIGHT - 30,
                         80);
        draw<TextButton>("Patterns", pins::BTN_UNDER_R, DISPLAY_WIDTH - 85,
                         Display::HEIGHT - 30, 80);

        draw<TextButton>("Pause", pins::BTN_UNDER_C, DISPLAY_WIDTH / 2 - 60,
                         Display::HEIGHT - 30, 120);

        draw<LinearRailGraph>(&this->settings.stroke, &this->settings.depth, -1,
                              Display::PageHeight - 40, Display::WIDTH);

        draw<DynamicText>(this->patternName, -1, Display::HEIGHT - 90);

        // Create a left encoder dial with Speed parameter
        std::map<String, float *> leftParams = {
            {"Speed", &this->settings.speed}};
        draw<EncoderDial>(EncoderDial::Props{
            .encoder = &leftEncoder,
            .parameters = leftParams,
            .focusedIndex = &this->leftFocusedIndex,
            .x = 0,
            .y = (int16_t)(Display::PageY +
                           10)});  // Both dials aligned 10px lower

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
            .y = (int16_t)(Display::PageY +
                           10)});  // Both dials aligned 10px lower
    }

    void onConnect() override {
        // when we connect, pull the current state from the device
        readJson<JsonObject>("state", [this](const JsonObject &state) {
            this->settings.speed = state["speed"].as<float>();
            this->settings.stroke = state["stroke"].as<float>();
            this->settings.sensation = state["sensation"].as<float>();
            this->settings.depth = state["depth"].as<float>();
            this->settings.pattern =
                static_cast<StrokePatterns>(state["pattern"].as<int>());

            ESP_LOGI(TAG,
                     "UPDATED SETTINGS: Speed: %d, Stroke: %d, Sensation: %d, "
                     "Depth: %d, Pattern: %d",
                     this->settings.speed, this->settings.stroke,
                     this->settings.sensation, this->settings.depth,
                     this->settings.pattern);
        });

        if (isFirstConnect || menu.empty()) {
            // And then we pull the patterns from the device
            readJson<JsonArray>("patterns", [this](const JsonArray &patterns) {
                // clear the patterns vector
                this->menu.clear();

                for (JsonVariant v : patterns) {
                    auto icon = researchAndDesireWaves;
                    std::string name = v["name"].as<const char *>();
                    std::string lowerName = name;
                    std::transform(lowerName.begin(), lowerName.end(),
                                   lowerName.begin(), ::tolower);

                    if (lowerName.find("simple") != std::string::npos) {
                        icon = researchAndDesireWaves;
                    } else if (lowerName.find("teasing") != std::string::npos) {
                        icon = researchAndDesireHeart;
                    } else if (lowerName.find("robo") != std::string::npos) {
                        icon = researchAndDesireTerminal;
                    } else if (lowerName.find("stop") != std::string::npos) {
                        icon = researchAndDesireHourglass03;
                    } else if (lowerName.find("insist") != std::string::npos) {
                        icon = researchAndDesireItalic02;
                    } else if (lowerName.find("deeper") != std::string::npos) {
                        icon = researchAndDesireArrowsRight;
                    } else if (lowerName.find("half") != std::string::npos) {
                        icon = researchAndDesireFaceWink;
                    }

                    // set pattern description char to idx.

                    int idx = v["idx"].as<int>();

                    std::optional<std::string> description = std::nullopt;
                    if (send("patternDescription", std::to_string(idx))) {
                        description = readString("patternDescription");
                        ESP_LOGI(TAG, "Description: %s", description->c_str());
                    }

                    ESP_LOGI(TAG, "Pattern: %s, %d", name.c_str(), idx);
                    this->menu.push_back(MenuItem{MenuItemE::DEVICE_MENU_ITEM,
                                                  name, icon, description,
                                                  .metaIndex = idx});
                }
            });
        }

        // finally, we set inital preferences and go to stroke engine mode
        send("speedKnobLimit", "false");
        send("command", "go:strokeEngine");
        vTaskDelay(pdMS_TO_TICKS(250));
        // TODO: A bug on AJ's dev unit requires two "go:strokeEngine" commands.
        send("command", "go:strokeEngine");
        vTaskDelay(pdMS_TO_TICKS(250));

        isConnected = true;
        isFirstConnect = false;
    }

    void onPause() override {
        playBuzzerPattern(BuzzerPattern::PAUSED);
        isPaused = true;
        setSpeed(0);
        leftEncoder.setEncoderValue(0);
        leftEncoder.setBoundaries(0, 0);
    }

    void onResume() override {
        playBuzzerPattern(BuzzerPattern::PLAY);
        leftEncoder.setBoundaries(0, 100);
        isPaused = false;
    }

    void onExit() override {
        // This forces the OSSM to go the menu screen.
        // this loop will not return until the device is diconnected or the menu
        // screen is reached.
        bool isInMenu = false;
        do {
            send("command", "go:menu");
            vTaskDelay(100 / portTICK_PERIOD_MS);
            readJson<String>("state", [this, &isInMenu](const String &state) {
                String currentState;
                stateMachine->visit_current_states([&currentState](auto state) {
                    currentState = state.c_str();
                });
                isInMenu = currentState.startsWith("menu");
            });
            vTaskDelay(100 / portTICK_PERIOD_MS);
        } while (isConnected && !isInMenu);
    }

    void onDeviceMenuItemSelected(int index) override { setPattern(index); }

    // Helper functions.
    bool setSpeed(int speed) {
        if (speed == settings.speed) {
            return true;
        }
        settings.speed = speed;
        speed = constrain(speed, 0, 100);
        return send("command",
                    std::string("set:speed:") + std::to_string(speed));
    }

    float getDepth() { return constrain(settings.depth, 0.0f, 100.0f); }

    bool setDepth(int depth) {
        if (depth == settings.depth) {
            return true;
        }
        settings.depth = depth;
        depth = constrain(depth, 0, 100);
        return send("command",
                    std::string("set:depth:") + std::to_string(depth));
    }

    float getStroke() { return constrain(settings.stroke, 0.0f, 100.0f); }

    bool setStroke(int stroke) {
        if (stroke == settings.stroke) {
            return true;
        }
        settings.stroke = stroke;
        stroke = constrain(stroke, 0, 100);
        return send("command",
                    std::string("set:stroke:") + std::to_string(stroke));
    }

    bool setSensation(int sensation) {
        if (sensation == settings.sensation) {
            return true;
        }
        settings.sensation = sensation;
        sensation = constrain(sensation, 0, 100);
        return send("command",
                    std::string("set:sensation:") + std::to_string(sensation));
    }

    bool setPattern(int pattern) {
        if (menu.empty()) {
            ESP_LOGW(TAG, "setPattern called but menu is empty");
            patternName = EMPTY_STRING;
            return false;
        }
        if (pattern == static_cast<int>(settings.pattern)) {
            return true;
        }
        settings.pattern = static_cast<StrokePatterns>(pattern);
        pattern = pattern % menu.size();
        int patternIdx = menu[pattern].metaIndex;
        patternName = menu[pattern].name;
        return send("command",
                    std::string("set:pattern:") + std::to_string(patternIdx));
    }

    void syncRightEncoder() {
        if (rightFocusedIndex == 0) {
            rightEncoder.setEncoderValue(settings.depth);
        } else if (rightFocusedIndex == 1) {
            rightEncoder.setEncoderValue(settings.sensation);
        } else if (rightFocusedIndex == 2) {
            rightEncoder.setEncoderValue(settings.stroke);
        }
    };

    void onLeftBumperClick() override {
        rightFocusedIndex = (rightFocusedIndex + 2) %
                            3;  // Safe decrement and wrap: 0->2, 1->0, 2->1
        syncRightEncoder();
    }

    void onRightBumperClick() override {
        rightFocusedIndex =
            (rightFocusedIndex + 1) % 3;  // Safe increment and wrap: 2->0
        syncRightEncoder();
    }

    void onRightEncoderChange(int value) override {
        if (rightFocusedIndex == 0) {
            setDepth(value);
        } else if (rightFocusedIndex == 1) {
            setSensation(value);
        } else if (rightFocusedIndex == 2) {
            setStroke(value);
        }
    }

    void onLeftEncoderChange(int value) override { setSpeed(value); }
};

#endif  // OSSM_DEVICE_H
