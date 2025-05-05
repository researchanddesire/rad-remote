#include "mcp.h"
#include "pins.h"
#include <Arduino.h>

Adafruit_MCP23X17 mcp;

void handleMCPInterrupt()
{
    Serial.println("MCP Interrupt");
}

bool initMCP()
{
    // Initialize MCP23017
    if (!mcp.begin_I2C())
    {
        Serial.println("Error: MCP23017 not found!");
        return false;
    }

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

    // Configure MCP23017 interrupts
    // mcp.setupInterrupts(true, false, LOW);                                                  // Enable interrupts, mirror INTA/B, active LOW
    // mcp.setupInterruptPin(pins::CENTER_BTN, CHANGE);                                        // Interrupt on any change
    // attachInterrupt(digitalPinToInterrupt(pins::MCP_INT_PIN), handleMCPInterrupt, FALLING); // INT A pin

    return true;
}
