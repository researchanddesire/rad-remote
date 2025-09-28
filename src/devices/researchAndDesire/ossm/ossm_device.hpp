#pragma once

#ifndef OSSM_DEVICE_H
#define OSSM_DEVICE_H

#include <ArduinoJson.h>
#include <components/DynamicText.h>
#include <components/EncoderDial.h>
#include <components/LinearRailGraph.h>
#include <components/TextButton.h>
#include <pages/menus.h>
#include <services/leds.h>

#include "../../device.h"

// Forward declaration for button counter reset function
extern void resetMiddleButtonCounter();

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
    
    // Reference to the pattern name display component for color control
    DynamicText* patternNameDisplay = nullptr;
    
    // Reference to the menu button for dynamic text/color changes
    TextButton* menuButton = nullptr;

    // Reference to the pause button for dynamic text/color changes
    TextButton* pauseStopButton = nullptr;
    
    // References to the tab buttons for dynamic styling
    TextButton* strokeTab = nullptr;
    TextButton* depthTab = nullptr;
    TextButton* sensationTab = nullptr;
    
    // References to encoder dials for dynamic arc coloring
    EncoderDial* leftEncoderDial = nullptr;
    EncoderDial* rightEncoderDial = nullptr;

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

        // Tab interface for right encoder settings - positioned near top of screen
        // Calculate tab dimensions: full width with 5px gaps, equally sized
        const int16_t tabY = Display::StatusbarHeight;  // Position near top without padding
        const int16_t tabHeight = 24;
        const int16_t tabGap = 0;
        const int16_t totalGaps = 2 * tabGap;  // 2 gaps between 3 tabs
        const int16_t tabWidth = (DISPLAY_WIDTH - totalGaps) / 3;
        
        // Draw the three tabs in the correct order: Depth, Sensation, Stroke
        depthTab = draw<TextButton>("Depth", NO_PIN, 0, tabY, tabWidth, tabHeight);  // rightFocusedIndex 0
        sensationTab = draw<TextButton>("Sensation", NO_PIN, tabWidth + tabGap, tabY, tabWidth, tabHeight);  // rightFocusedIndex 1
        strokeTab = draw<TextButton>("Stroke", NO_PIN, 2 * (tabWidth + tabGap), tabY, tabWidth, tabHeight);  // rightFocusedIndex 2
        
        // Update tab appearance based on current focus
        updateTabAppearance();

        // Top bumpers - positioned intentionally with negative margin to square off with screen edge
        draw<TextButton>("<<", pins::BTN_L_SHOULDER, -5, -5);
        draw<TextButton>(">>", pins::BTN_R_SHOULDER, DISPLAY_WIDTH - 65, -5);

        // Bottom bumpers - positioned with margin to prevent border cutoff
        //NOTE: This button is currently non-functional as OSSM has no device menu and is set to NO_PIN to disable.
        menuButton = draw<TextButton>("Menu", NO_PIN, -5, Display::HEIGHT - 30,
                         90);
        draw<TextButton>("Patterns", pins::BTN_UNDER_R, DISPLAY_WIDTH - 85,
                         Display::HEIGHT - 30, 90);

        pauseStopButton = draw<TextButton>("Pause", pins::BTN_UNDER_C, DISPLAY_WIDTH / 2 - 60,
                         Display::HEIGHT - 30, 120);

        // Set initial disabled state for menu button (enabled only when paused)
        if (menuButton) {
            menuButton->setColors(Colors::disabled, Colors::black);
        }

        draw<LinearRailGraph>(&this->settings.stroke, &this->settings.depth, -1,
                              Display::PageHeight - 30, Display::WIDTH-20, 20);

        patternNameDisplay = draw<DynamicText>(this->patternName, -1, Display::HEIGHT - 70);

        // Create a left encoder dial with Speed parameter
        std::map<String, float *> leftParams = {
            {"Speed", &this->settings.speed}};
        leftEncoderDial = draw<EncoderDial>(EncoderDial::Props{
            .encoder = &leftEncoder,
            .parameters = leftParams,
            .focusedIndex = &this->leftFocusedIndex,
            .x = 0 + 5,
            .y = (int16_t)(Display::PageY +
                           40),  // Both dials aligned 10px lower
            .mapToLeftLed = true});  // Map to left LED

        // Set the left encoder dial color to purple (always active since it only has one parameter)
        if (leftEncoderDial) {
            std::vector<uint16_t> leftColors = {Colors::speed};  // Purple for Speed
            leftEncoderDial->setParameterColors(leftColors);
        }

        // Create a right encoder dial with all parameters
        std::map<String, float *> rightParams = {
            {"Depth", &this->settings.depth},
            {"Sens.", &this->settings.sensation},
            {"Stroke", &this->settings.stroke}};
        rightEncoderDial = draw<EncoderDial>(EncoderDial::Props{
            .encoder = &rightEncoder,
            .parameters = rightParams,
            .focusedIndex = &this->rightFocusedIndex,
            .x = (int16_t)(DISPLAY_WIDTH - 90 -5),
            .y = (int16_t)(Display::PageY +
                           40),  // Both dials aligned 10px lower
            .mapToRightLed = true});  // Map to right LED

        // Set up right encoder dial colors to match tab order (Depth, Sensation, Stroke)
        updateEncoderDialColors();
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

                updatePatternNameFromState();
            });

            setMiddleLed(Colors::white, 50);
            // Disable menu button in default state
            if (menuButton) {
                menuButton->setColors(Colors::disabled, Colors::black);
            }
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

    void onPause(bool fullStop = false) override {
        playBuzzerPattern(BuzzerPattern::PAUSED);
        isPaused = true;
        setSpeed(0);
        leftEncoder.setEncoderValue(0);

        patternName = "Paused";
        
        // Set the pattern name color to red when paused
        if (patternNameDisplay) {
            patternNameDisplay->setColor(Colors::red);
        }
        
        // Change pause button to red STOP button
        if (pauseStopButton) {
            pauseStopButton->setText("STOP");
            pauseStopButton->setColors(Colors::red, Colors::white);
        }

        // TODO: Uncomment this when functionality is added for OSSM Device Menu
        // Enable menu button when paused
        // if (menuButton) {
        //     menuButton->setColors(Colors::textBackground, Colors::black);
        // }

        // Set middle LED to red to indicate STOP state
        setMiddleLed(Colors::red, 255);

        // Reset all play parameters to defaults, state will also be changed
        if (fullStop) {
            setDepth(0);
            setStroke(10);
            setSensation(50);
            rightEncoder.setEncoderValue(0);

            //TODO: Anything else for consideration in full stop before swapping state?
        }
    }

    void onResume() override {
        // playBuzzerPattern(BuzzerPattern::PLAY);
        leftEncoder.setBoundaries(0, 100);
        isPaused = false;
        resetMiddleButtonCounter();
        setMiddleLed(Colors::white, 50);
        
        // Reset the pattern name color to default when resuming
        if (patternNameDisplay) {
            patternNameDisplay->setColor(Colors::textBackground);
        }
        
        // Change button back to default Pause button styling
        if (pauseStopButton) {
            pauseStopButton->setText("Pause");
            pauseStopButton->setColors(Colors::textBackground, Colors::black);
        }

        // Disable menu button when resumed
        if (menuButton) {
            menuButton->setColors(Colors::disabled, Colors::black);
        }

        updatePatternNameFromState();

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

        //release all leds back to global control
        for (uint8_t i = 0; i < pins::NUM_LEDS; i++) {
            releaseIndividualLed(i);
        }
    }

    void onDeviceMenuItemSelected(int index) override { setPattern(index); }

    void drawDeviceMenu() override {
        activeMenu = &menu;
        activeMenuCount = menu.size();

        // Find the menu index that corresponds to the current pattern
        currentOption = 0;  // Default to first item
        for (int i = 0; i < menu.size(); i++) {
            if (menu[i].metaIndex == static_cast<int>(settings.pattern)) {
                currentOption = i;
                break;
            }
        }

        drawMenu();
    }

    // Helper functions.
    bool setSpeed(int speed) {
        // Send if value changed OR if encoder has moved
        // (even if sent value claims to be the same as previous)
        if (speed == settings.speed && !hasLeftEncoderChanged(true)) {
            return true;
        }
        settings.speed = speed;
        speed = constrain(speed, 0, 100);
        return send("command",
                    std::string("set:speed:") + std::to_string(speed));
    }

    float getDepth() { return constrain(settings.depth, 0.0f, 100.0f); }

    bool setDepth(int depth) {
        // Send if value changed OR if encoder has moved
        // (even if sent value claims to be the same as previous)
        if (depth == settings.depth && !hasRightEncoderChanged(true)) {
            return true;
        }
        settings.depth = depth;
        depth = constrain(depth, 0, 100);
        return send("command",
                    std::string("set:depth:") + std::to_string(depth));
    }

    float getStroke() { return constrain(settings.stroke, 0.0f, 100.0f); }

    bool setStroke(int stroke) {
        // Send if value changed OR if encoder has moved
        // (even if sent value claims to be the same as previous)
        if (stroke == settings.stroke && !hasRightEncoderChanged(true)) {
            return true;
        }
        settings.stroke = stroke;
        stroke = constrain(stroke, 0, 100);
        return send("command",
                    std::string("set:stroke:") + std::to_string(stroke));
    }

    bool setSensation(int sensation) {
        // Send if value changed OR if encoder has moved (even if value stayed
        // same due to boundaries)
        if (sensation == settings.sensation && !hasRightEncoderChanged(true)) {
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
        
        // Reset color to default when selecting a new pattern
        if (patternNameDisplay) {
            patternNameDisplay->setColor(Colors::textForeground);
        }
        
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
        updateTabAppearance();
        updateEncoderDialColors();
    }

    void onRightBumperClick() override {
        rightFocusedIndex =
            (rightFocusedIndex + 1) % 3;  // Safe increment and wrap: 2->0
        syncRightEncoder();
        updateTabAppearance();
        updateEncoderDialColors();
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

    void onLeftEncoderChange(int value) override {
        setSpeed(value);
        if(isPaused && value > 0) {
            onResume();
        }
    }

    // Enable persistent encoder monitoring for safety-critical speed control
    bool needsPersistentLeftEncoderMonitoring() const override { return true; }

    // Provide current speed value for status display
    int getCurrentLeftEncoderValue() const override {
        return static_cast<int>(settings.speed);
    }

    // Provide the left encoder parameter name for display
    const char *getLeftEncoderParameterName() const override { return "Speed"; }

  private:
    void updatePatternNameFromState() {
        // Find the pattern name that corresponds to the current pattern from
        // BLE state
        for (const auto &menuItem : menu) {
            if (menuItem.metaIndex == static_cast<int>(settings.pattern)) {
                patternName = menuItem.name;
                return;
            }
        }

        // If no match found, keep default or set to empty
        ESP_LOGW(TAG, "Could not find pattern name for pattern index: %d",
                 static_cast<int>(settings.pattern));
    }

    void updateTabAppearance() {
        if (!strokeTab || !depthTab || !sensationTab) return;
        
        // Reset all tabs to default appearance
        strokeTab->setColors(Colors::disabled, Colors::black);
        depthTab->setColors(Colors::disabled, Colors::black);
        sensationTab->setColors(Colors::disabled, Colors::black);

        // Highlight the active tab based on rightFocusedIndex
        // rightFocusedIndex: 0=Depth, 1=Sensation, 2=Stroke
        if (rightFocusedIndex == 0) {
            depthTab->setColors(Colors::depth, Colors::white);  // Active: red background, white text
        } else if (rightFocusedIndex == 1) {
            sensationTab->setColors(Colors::sensation, Colors::white);
        } else if (rightFocusedIndex == 2) {
            strokeTab->setColors(Colors::stroke, Colors::white);
        }
    }

    void updateEncoderDialColors() {
        if (!rightEncoderDial) return;
        
        // Reset all arc colors to default white
        std::vector<uint16_t> arcColors = {ST77XX_WHITE, ST77XX_WHITE, ST77XX_WHITE};
        
        // Set the active parameter's arc color to match the tab color
        // rightFocusedIndex: 0=Depth, 1=Sensation, 2=Stroke
        if (rightFocusedIndex == 0) {
            arcColors[0] = Colors::depth;     // Depth arc gets depth color
        } else if (rightFocusedIndex == 1) {
            arcColors[1] = Colors::sensation; // Sensation arc gets sensation color
        } else if (rightFocusedIndex == 2) {
            arcColors[2] = Colors::stroke;    // Stroke arc gets stroke color
        }
        
        rightEncoderDial->setParameterColors(arcColors);
    }
};

#endif  // OSSM_DEVICE_H
