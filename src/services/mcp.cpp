#include "mcp.h"
#include "pins.h"
#include <Arduino.h>
#include "state/remote.h"

Adafruit_MCP23X17 mcp;

static bool interrupted = false;
void handleMCPInterrupt()
{
    interrupted = true;
}
void mcpTask(void *pvParameters)
{
    while (true)
    {
        if (!interrupted)
        {
            vTaskDelay(100);
            continue;
        }

        // Check button states
        bool leftBtn = !mcp.digitalRead(pins::LEFT_BTN); // Inverted due to pullup
        bool centerBtn = !mcp.digitalRead(pins::CENTER_BTN);
        bool rightBtn = !mcp.digitalRead(pins::RIGHT_BTN);

        if (leftBtn)
            stateMachine->process_event(left_button_pressed());
        if (centerBtn)
            stateMachine->process_event(middle_button_pressed());
        if (rightBtn)
            stateMachine->process_event(right_button_pressed());

        interrupted = false;
    }
}

bool initMCP()
{
    // Initialize MCP23017
    if (!mcp.begin_I2C())
    {
        Serial.println("Error: MCP23017 not found!");
        return false;
    }

    Serial.println("MCP23017 initialized successfully");

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

    mcp.setupInterrupts(true, false, LOW); // Enable interrupts, mirror INTA/B, active LOW
    mcp.setupInterruptPin(pins::CENTER_BTN, CHANGE);
    mcp.setupInterruptPin(pins::RIGHT_BTN, CHANGE);
    mcp.setupInterruptPin(pins::LEFT_BTN, CHANGE);
    attachInterrupt(digitalPinToInterrupt(pins::MCP_INT_PIN), handleMCPInterrupt, FALLING); // INT A pin

    xTaskCreatePinnedToCore(
        mcpTask,
        "mcpTask",
        2048,
        NULL,
        5,
        NULL, 0);

    return true;
}
