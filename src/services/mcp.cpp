#include "mcp.h"
#include "pins.h"
#include <Arduino.h>
#include "state/remote.h"

Adafruit_MCP23X17 mcp;

struct ButtonConfig
{
    const char *name;
    uint8_t pin;
    unsigned long lastDebounceTime;
    bool lastState;
    bool lastStableState;
    void (*event)();
};

void mcpTask(void *pvParameters)
{
    const unsigned long DEBOUNCE_DELAY = 50; // milliseconds

    ButtonConfig buttons[] = {
        {"Left", pins::LEFT_BTN, 0, false, false, []()
         { stateMachine->process_event(left_button_pressed()); }},

        {"Center", pins::CENTER_BTN, 0, false, false, []()
         { stateMachine->process_event(middle_button_pressed()); }},

        {"Right", pins::RIGHT_BTN, 0, false, false, []()
         { stateMachine->process_event(right_button_pressed()); }}};

    while (true)
    {
        vTaskDelay(10);
        unsigned long currentTime = millis();

        for (auto &btn : buttons)
        {
            bool currentState = !mcp.digitalRead(btn.pin);

            // Update debounce time if state changed
            if (currentState != btn.lastState)
            {
                btn.lastDebounceTime = currentTime;
                btn.lastState = currentState;
                continue;
            }

            // Skip if not enough time has passed for debounce
            if ((currentTime - btn.lastDebounceTime) <= DEBOUNCE_DELAY)
            {
                continue;
            }

            // Handle button press
            if (currentState && !btn.lastStableState)
            {
                ESP_LOGD("MCP", "%s button pressed", btn.name);
                btn.event();
            }

            btn.lastStableState = currentState;
            btn.lastState = currentState;
        }
    }
}

bool initMCP()
{
    // Initialize MCP23017
    if (!mcp.begin_I2C())
    {
        ESP_LOGD("MCP", "Error: MCP23017 not found!");
        return false;
    }

    ESP_LOGD("MCP", "MCP23017 initialized successfully");

    // Configure MCP23017 pins
    // Input pins with pull-up
    mcp.pinMode(pins::RIGHT_SHOULDER_BTN, INPUT_PULLUP);
    mcp.pinMode(pins::LEFT_SHOULDER_BTN, INPUT_PULLUP);
    mcp.pinMode(pins::LEFT_BTN, INPUT_PULLUP);
    mcp.pinMode(pins::CENTER_BTN, INPUT_PULLUP);
    mcp.pinMode(pins::RIGHT_BTN, INPUT_PULLUP);
    mcp.pinMode(pins::EXT_IO3, INPUT_PULLUP);
    mcp.pinMode(pins::EXT_IO4, INPUT_PULLUP);
    mcp.pinMode(pins::GYRO_INT1, INPUT_PULLUP);
    mcp.pinMode(pins::GYRO_INT2, INPUT_PULLUP);

    // Output pins
    mcp.pinMode(pins::BUZZER, OUTPUT);
    mcp.pinMode(pins::VIBRATOR, OUTPUT);
    mcp.pinMode(pins::REGULATOR_EN, OUTPUT);
    mcp.pinMode(pins::FUEL_GAUGE, OUTPUT);

    // mcp.setupInterrupts(true, false, LOW); // Enable interrupts, mirror INTA/B, active LOW
    // mcp.setupInterruptPin(pins::CENTER_BTN, CHANGE);
    // mcp.setupInterruptPin(pins::RIGHT_BTN, CHANGE);
    // mcp.setupInterruptPin(pins::LEFT_BTN, CHANGE);
    // attachInterrupt(digitalPinToInterrupt(pins::MCP_INT_PIN), handleMCPInterrupt, FALLING); // INT A pin

    // Verify interrupt configuration
    ESP_LOGI("MCP", "Verifying interrupt configuration...");
    ESP_LOGI("MCP", "Interrupt pin states - Left: %d, Center: %d, Right: %d",
             mcp.digitalRead(pins::LEFT_BTN),
             mcp.digitalRead(pins::CENTER_BTN),
             mcp.digitalRead(pins::RIGHT_BTN));

    xTaskCreatePinnedToCore(
        mcpTask,
        "mcpTask",
        10 * configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 1,
        NULL, 0);

    return true;
}
