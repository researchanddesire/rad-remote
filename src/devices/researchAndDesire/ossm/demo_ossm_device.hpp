#pragma once

#ifndef DEMO_OSSM_DEVICE_H
#define DEMO_OSSM_DEVICE_H

#include "ossm_device.hpp"

class DemoOSSM : public OSSM {
  private:
    bool isDemoMode = true;
    bool connectionTaskStarted = false;

  public:
    explicit DemoOSSM() : OSSM(nullptr) {
        // Set default demo values
        settings.speed = 10;
        settings.sensation = 50; 
        settings.depth = 50;
        settings.stroke = 50;
        settings.pattern = StrokePatterns::SimpleStroke;
        
        isConnected = true; // Fake the connection
        patternName = DEFAULT_OSSM_PATTERN_NAME;
        
        // Create a fake pattern menu
        menu.clear();
        menu.push_back(MenuItem{MenuItemE::DEVICE_MENU_ITEM, "Simple Stroke", researchAndDesireWaves, std::nullopt, -1, -1, 0});
        menu.push_back(MenuItem{MenuItemE::DEVICE_MENU_ITEM, "Teasing Pounding", researchAndDesireHeart, std::nullopt, -1, -1, 1});
        menu.push_back(MenuItem{MenuItemE::DEVICE_MENU_ITEM, "Robo Stroke", researchAndDesireTerminal, std::nullopt, -1, -1, 2});
        menu.push_back(MenuItem{MenuItemE::DEVICE_MENU_ITEM, "Half & Half", researchAndDesireFaceWink, std::nullopt, -1, -1, 3});
        menu.push_back(MenuItem{MenuItemE::DEVICE_MENU_ITEM, "Deeper", researchAndDesireArrowsRight, std::nullopt, -1, -1, 4});
        menu.push_back(MenuItem{MenuItemE::DEVICE_MENU_ITEM, "Stop & Go", researchAndDesireHourglass03, std::nullopt, -1, -1, 5});
        menu.push_back(MenuItem{MenuItemE::DEVICE_MENU_ITEM, "Insist", researchAndDesireItalic02, std::nullopt, -1, -1, 6});
        
        // Don't start the BLE connection task for demo mode
        connectionTaskStarted = false;
    }

    const char *getName() override { return "OSSM (Demo)"; }
    
    // Override connection methods to prevent actual BLE operations
    void onConnect() override {
        // Skip all BLE operations from parent class
        // Just setup the initial values and UI state without calling private methods
        isFirstConnect = false;
        
        setMiddleLed(Colors::white, 50);
        // Enable menu button for demo mode (unlike real OSSM which starts disabled)
        if (menuButton) {
            menuButton->setColors(Colors::textBackground, Colors::black);
        }
        
        // Skip the BLE commands that parent class would send
        ESP_LOGI("DemoOSSM", "Demo OSSM connected - no BLE operations");
    }

    void onExit() override {
        // Skip BLE operations for demo mode - don't call parent's onExit
        // which would try to communicate with a real device
        ESP_LOGI("DemoOSSM", "Demo OSSM exiting - no BLE operations needed");
        
        // Just cleanup LEDs like the parent would do
        for (uint8_t i = 0; i < pins::NUM_LEDS; i++) {
            releaseIndividualLed(i);
        }
    }
    
    void drawControls() override {
        // Call parent's drawControls to set up the UI
        OSSM::drawControls();
        
        // Demo mode has full functionality including patterns menu
        // No need to disable any buttons
    }

    // Override encoder change methods to prevent BLE communication
    void onRightEncoderChange(int value) override {
        // Update local values but don't send to device
        ESP_LOGI("DemoOSSM", "Demo mode - right encoder changed to: %d", value);
        // The parent class handles the UI updates
    }

    void onLeftEncoderChange(int value) override {
        // Update speed settings and handle pause/resume like real OSSM
        setSpeed(value);
        if (isPaused && value > 0) {
            onResume();
        }
        ESP_LOGI("DemoOSSM", "Demo mode - left encoder changed to: %d (speed)", value);
    }

    void onDeviceMenuItemSelected(int index) override {
        // Handle pattern selection in demo mode
        if (index >= 0 && index < menu.size()) {
            settings.pattern = static_cast<StrokePatterns>(menu[index].metaIndex);
            patternName = menu[index].name;
            ESP_LOGI("DemoOSSM", "Demo mode - pattern selected: %s", patternName.c_str());
            
            // Update pattern name display if it exists
            if (patternNameDisplay) {
                // DynamicText references a string, so update our patternName
                // The display will automatically update on next draw
                ESP_LOGI("DemoOSSM", "Updated pattern name to: %s", patternName.c_str());
            }
        }
    }

    // Check if we're in demo mode
    bool isInDemoMode() const {
        return isDemoMode;
    }

  protected:
    // Override the Device class protected methods to prevent BLE operations
    bool send(const std::string &command, const std::string &value) {
        ESP_LOGI("DemoOSSM", "Demo mode - would send command: %s = %s", command.c_str(), value.c_str());
        return true; // Always return success
    }

    std::string readString(const std::string &characteristicName) {
        ESP_LOGI("DemoOSSM", "Demo mode - reading %s", characteristicName.c_str());
        
        if (characteristicName == "state") {
            // Return current demo state as JSON
            JsonDocument doc;
            doc["speed"] = settings.speed;
            doc["stroke"] = settings.stroke;
            doc["sensation"] = settings.sensation;
            doc["depth"] = settings.depth;
            doc["pattern"] = static_cast<int>(settings.pattern);
            
            String jsonString;
            serializeJson(doc, jsonString);
            return jsonString.c_str();
        } else if (characteristicName == "patterns") {
            // Return demo patterns as JSON array
            JsonDocument doc;
            JsonArray patterns = doc.to<JsonArray>();
            
            // Use the newer add<JsonObject>() method instead of deprecated createNestedObject()
            JsonObject pattern1 = patterns.add<JsonObject>();
            pattern1["name"] = "Simple Stroke";
            pattern1["index"] = 0;
            
            JsonObject pattern2 = patterns.add<JsonObject>();
            pattern2["name"] = "Teasing Pounding";
            pattern2["index"] = 1;
            
            JsonObject pattern3 = patterns.add<JsonObject>();
            pattern3["name"] = "Robo Stroke";
            pattern3["index"] = 2;
            
            JsonObject pattern4 = patterns.add<JsonObject>();
            pattern4["name"] = "Half & Half";
            pattern4["index"] = 3;
            
            JsonObject pattern5 = patterns.add<JsonObject>();
            pattern5["name"] = "Deeper";
            pattern5["index"] = 4;
            
            JsonObject pattern6 = patterns.add<JsonObject>();
            pattern6["name"] = "Stop & Go";
            pattern6["index"] = 5;
            
            JsonObject pattern7 = patterns.add<JsonObject>();
            pattern7["name"] = "Insist";
            pattern7["index"] = 6;
            
            String jsonString;
            serializeJson(doc, jsonString);
            return jsonString.c_str();
        }
        
        return "{}";
    }
};

#endif // DEMO_OSSM_DEVICE_H