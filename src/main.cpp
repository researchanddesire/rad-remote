#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/timer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include "pins.h"
#include "constants.h"
#include "components/TextButton.h"
#include "services/display.h"
#include "services/mcp.h"
#include "services/leds.h"
#include "services/battery.h"
#include "services/imu.h"
#include "services/encoder.h"
#include "components/EncoderDial.h"
#include "utils/buzzer.h"
#include "utils/vibrator.h"
#include "services/coms.h"
#include "state/remote.h"

// Add these variables at the top with other control variables
unsigned long lastRainbowUpdate = 0;
uint16_t rainbowIndex = 0;
int16_t lastBrightness = -1;
int16_t lastSpeed = -1;
bool lastVibratorState = false;
bool lastBuzzerState = false;
int16_t lastLeftEncoder = -1;
int16_t lastRightEncoder = -1;
bool lastRightShoulder = false;
bool lastLeftShoulder = false;
bool lastLeftBtn = false;
bool lastCenterBtn = false;
bool lastRightBtn = false;

// Add to control variables section
bool enableSleep = true; // Flag to control sleep functionality

// Control variables
uint8_t rainbowSpeed = 20; // Initial speed (ms delay)
unsigned long lastDebugTime = 0;
static unsigned long lastToggle = 0;
static long lastBuzzerTime = 0;
static long lastVibratorTime = 0;

// Top bumpers
TextButton topLeftBumper("modify", pins::LEFT_SHOULDER_BTN, 0, 0);
TextButton topRightBumper("modify", pins::RIGHT_SHOULDER_BTN, DISPLAY_WIDTH - 60, 0);

// Bottom bumpers
TextButton bottomLeftBumper("Home", pins::LEFT_BTN, 0, DISPLAY_HEIGHT - 25);
TextButton bottomRightBumper("Patterns", pins::RIGHT_BTN, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT - 25);

TextButton centerButton("STOP", pins::CENTER_BTN, DISPLAY_WIDTH / 2 - 60, DISPLAY_HEIGHT - 25, 120);

// Add to control variables section
static TaskHandle_t buttonUpdateTaskHandle = NULL;

// Create a left encoder dial with Speed parameter
std::map<String, int> leftParams = {
    {"Speed", 0}};
EncoderDial leftDial(leftParams, "", true, 0, DISPLAY_HEIGHT / 2 - 30);

// Create a right encoder dial with all parameters
std::map<String, int> rightParams = {
    {"Stroke", 0},
    {"Depth", 0},
    {"Sens.", 0}};
EncoderDial rightDial(rightParams, "", false, DISPLAY_WIDTH - 90, DISPLAY_HEIGHT / 2 - 30);

// Add these variables at the top with other global variables
int currentRightParamIndex = 0;
const String rightParamNames[] = {"Stroke", "Depth", "Sens."};

void scanI2CDevices()
{
    Serial.println("\nScanning I2C bus...");
    byte error, address;
    int deviceCount = 0;

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.printf("I2C device found at address 0x%02X\n", address);
            deviceCount++;

            // Print known device names
            switch (address)
            {
            case 0x20:
                Serial.println("  -> MCP23017 (GPIO Expander)");
                break;
            case 0x36:
                Serial.println("  -> MAX17048 (Battery Monitor)");
                break;
            case 0x55:
                Serial.println("  -> BQ27220 (Battery Fuel Gauge)");
                break;
            case 0x6A:
                Serial.println("  -> LSM6DS3 (Accelerometer)");
                break;
            case 0x6B:
                Serial.println("  -> LSM6DS3 (Accelerometer - Alternate Address)");
                break;
            default:
                Serial.println("  -> Unknown device");
                break;
            }
        }
    }

    if (deviceCount == 0)
    {
        Serial.println("No I2C devices found!");
    }
    else
    {
        Serial.printf("Found %d I2C device(s)\n", deviceCount);
    }
    Serial.println();
}

void setup()
{
    Serial.begin(115200);

    // Initialize I2C
    Wire.begin(pins::I2C_SDA, pins::I2C_SCL);

    // Scan for I2C devices
    scanI2CDevices();

    initBattery(500);
    initIMUService();
    initDisplay();
    initMCP();
    initEncoderService();
    initBuzzer();
    initVibrator();

    // Initialize ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    initESPNow();
    xTaskCreatePinnedToCore(
        [](void *pvParameters)
        {
            while (true)
            {
                sendESPNow(settings);
                vTaskDelay(
                    10 / portTICK_PERIOD_MS); // 100ms delay between broadcasts
            }
        },
        "espnowTask", 4 * configMINIMAL_STACK_SIZE, nullptr, 1, nullptr, 0);
}

void loop()
{
    updateBatteryStatus();
    updateIMUReadings();

    settings.speed = 100 - leftEncoder.readEncoder();
    leftDial.setParameter(settings.speed);

    if (rightDial.getFocusedIndex() == 0)
    {
        settings.stroke = 100 - rightEncoder.readEncoder();
        rightDial.setParameter(settings.stroke);
    }
    else if (rightDial.getFocusedIndex() == 1)
    {
        settings.depth = 100 - rightEncoder.readEncoder();
        rightDial.setParameter(settings.depth);
    }
    else if (rightDial.getFocusedIndex() == 2)
    {
        settings.sensation = 100 - rightEncoder.readEncoder();
        rightDial.setParameter(settings.sensation);
    }

    // Handle shoulder button presses for parameter cycling
    static bool lastLeftShoulderState = HIGH;
    static bool lastRightShoulderState = HIGH;

    bool currentLeftShoulderState = mcp.digitalRead(pins::LEFT_SHOULDER_BTN);
    bool currentRightShoulderState = mcp.digitalRead(pins::RIGHT_SHOULDER_BTN);

    // Check for falling edge (button press) on right shoulder
    if (currentRightShoulderState == LOW && lastRightShoulderState == HIGH)
    {
        // Increment focus
        int newParameter = rightDial.incrementFocus();
        rightEncoder.setEncoderValue(100 - newParameter);
    }

    // Check for falling edge (button press) on left shoulder
    if (currentLeftShoulderState == LOW && lastLeftShoulderState == HIGH)
    {
        // Decrement focus
        int newParameter = rightDial.decrementFocus();
        rightEncoder.setEncoderValue(100 - newParameter);
    }

    // Store current states for next iteration
    lastLeftShoulderState = currentLeftShoulderState;
    lastRightShoulderState = currentRightShoulderState;

    // Print debug info every 2 seconds
    if (millis() - lastDebugTime >= 50)
    {
        topLeftBumper.tick();
        topRightBumper.tick();

        bottomLeftBumper.tick();
        bottomRightBumper.tick();
        centerButton.tick();

        leftDial.tick();
        rightDial.tick();

        lastDebugTime = millis();
    }

    stopVibrator();
}
