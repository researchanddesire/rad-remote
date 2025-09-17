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
#include <NimBLEDevice.h>
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
#include "components/AnimatedIcons.h"
#include <OneButton.h>

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

OneButton leftShoulderBtn;
OneButton rightShoulderBtn;
OneButton underLeftBtn;
OneButton underCenterBtn;
OneButton underRightBtn;

void setup()
{
    Serial.begin(115200);

    pinMode(pins::VIBRATOR_PIN, OUTPUT);

    // init buttons

    leftShoulderBtn = OneButton(pins::BTN_L_SHOULDER, true, true);
    leftShoulderBtn.attachClick([]()
                                { stateMachine->process_event(left_button_pressed()); });
    rightShoulderBtn = OneButton(pins::BTN_R_SHOULDER, true, true);
    rightShoulderBtn.attachClick([]()
                                 { stateMachine->process_event(right_button_pressed()); });
    underLeftBtn = OneButton(pins::BTN_UNDER_L, true, true);
    underLeftBtn.attachClick([]()
                             { stateMachine->process_event(left_button_pressed()); });
    underCenterBtn = OneButton(pins::BTN_UNDER_C, true, true);
    underCenterBtn.attachClick([]()
                               { stateMachine->process_event(middle_button_pressed()); });
    underRightBtn = OneButton(pins::BTN_UNDER_R, true, true);
    underRightBtn.attachClick([]()
                              { stateMachine->process_event(right_button_pressed()); });

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
    initBLE();
    initStateMachine();
    updateBatteryStatus();
    updateIMUReadings();

    setupAnimatedIcons();

    xTaskCreatePinnedToCore(
        [](void *pvParameters)
        {
            while (true)
            {
                vTaskDelay(10);
                leftShoulderBtn.tick();
                rightShoulderBtn.tick();
                underLeftBtn.tick();
                underCenterBtn.tick();
                underRightBtn.tick();
            }
        },
        "buttonTask",
        4 * configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 1,
        NULL, 0);
}

void loop()
{
    // delete the loop task. Everything is managed by the state machine now.
    vTaskDelete(NULL);
}
