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
#include "services/display.h"
#include "services/mcp.h"
#include "services/leds.h"
#include "services/battery.h"
#include "services/imu.h"
#include "services/encoder.h"
#include "utils/buzzer.h"
#include "utils/vibrator.h"
#include "services/coms.h"
#include "state/remote.h"
#include "esp_log.h"

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

// Add to control variables section
static TaskHandle_t buttonUpdateTaskHandle = NULL;

// Add these variables at the top with other global variables
int currentRightParamIndex = 0;
const String rightParamNames[] = {"Stroke", "Depth", "Sens."};
void scanI2CDevices()
{
    ESP_LOGD("I2C", "Scanning I2C bus...");
    byte error, address;
    int deviceCount = 0;

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            ESP_LOGD("I2C", "Device found at address 0x%02X", address);
            deviceCount++;

            // Print known device names
            switch (address)
            {
            case 0x20:
                ESP_LOGD("I2C", "  -> MCP23017 (GPIO Expander)");
                break;
            case 0x36:
                ESP_LOGD("I2C", "  -> MAX17048 (Battery Monitor)");
                break;
            case 0x55:
                ESP_LOGD("I2C", "  -> BQ27220 (Battery Fuel Gauge)");
                break;
            case 0x6A:
                ESP_LOGD("I2C", "  -> LSM6DS3 (Accelerometer)");
                break;
            case 0x6B:
                ESP_LOGD("I2C", "  -> LSM6DS3 (Accelerometer - Alternate Address)");
                break;
            default:
                ESP_LOGD("I2C", "  -> Unknown device");
                break;
            }
        }
    }

    if (deviceCount == 0)
    {
        ESP_LOGD("I2C", "No I2C devices found!");
    }
    else
    {
        ESP_LOGD("I2C", "Found %d I2C device(s)", deviceCount);
    }
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
        ESP_LOGD("COMS", "Error initializing ESP-NOW");
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

    initStateMachine();
    updateBatteryStatus();
    updateIMUReadings();

    stopVibrator();
}

void loop()
{
    // delete the loop task. Everything is managed by the state machine now.
    vTaskDelete(NULL);
}
