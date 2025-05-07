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

// ESP-NOW broadcast address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure to send data
typedef struct struct_message
{
    uint8_t brightness;
    uint8_t speed;
} struct_message;

struct_message myData;

// Add to control variables section
unsigned long lastEspNowTime = 0;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        Serial.println("ESP-NOW message sent successfully");
    }
    else
    {
        Serial.println("ESP-NOW message failed to send");
    }
}

// Top bumpers
TextButton topLeftBumper("modify", pins::LEFT_SHOULDER_BTN, 0, 0);
TextButton topRightBumper("modify", pins::RIGHT_SHOULDER_BTN, DISPLAY_WIDTH - 60, 0);

// Bottom bumpers
TextButton bottomLeftBumper("Home", pins::LEFT_BTN, 0, DISPLAY_HEIGHT - 25);
TextButton bottomRightBumper("Patterns", pins::RIGHT_BTN, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT - 25);

TextButton centerButton("STOP", pins::CENTER_BTN, DISPLAY_WIDTH / 2 - 60, DISPLAY_HEIGHT - 25, 120);

// Add to control variables section
static TaskHandle_t buttonUpdateTaskHandle = NULL;

// Create a left encoder dial
EncoderDial leftDial("Speed", "", true, 0, DISPLAY_HEIGHT / 2 - 30);

// Create a right encoder dial
EncoderDial rightDial("Depth", "", false, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT / 2 - 30);

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

    // Set BQ27220 full charge capacity to 500mAh
    setFullChargeCapacity(500);

    // Initialize IMU service
    if (!initIMUService())
    {
        Serial.println("Failed to initialize IMU service!");
        while (1)
        {
            delay(10);
        }
    }

    // Initialize screen
    pinMode(pins::TFT_BACKLIGHT, OUTPUT);
    digitalWrite(pins::TFT_BACKLIGHT, HIGH);
    SPI.begin(pins::TFT_SCK, -1, pins::TFT_SDA, -1);
    tft.init(240, 320); // Initialize with screen dimensions
    tft.setRotation(1); // Landscape mode
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);

    // Initialize MCP23017
    if (!initMCP())
    {
        while (1)
            ;
    }

    // Initialize battery service
    if (!initBatteryService())
    {
        Serial.println("Failed to initialize battery service!");
    }

    // Initialize encoder service
    initEncoderService();

    // Initialize buzzer
    initBuzzer();

    // Initialize ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(OnDataSent);

    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return;
    }

    // Initialize vibrator
    initVibrator();
}

void broadcastEspNow()
{
    if (millis() - lastEspNowTime >= 500)
    { // Broadcast every 500ms
        lastEspNowTime = millis();
        WiFi.setSleep(false);
        // Update data structure
        myData.brightness = brightness;
        myData.speed = rainbowSpeed;

        // Send data
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

        if (result == ESP_OK)
        {
            Serial.printf("Broadcasting - Brightness: %d, Speed: %d\n", brightness, rainbowSpeed);
        }
        else
        {
            Serial.println("Error sending the data");
            Serial.println(result);
            Serial.println(esp_err_to_name(result));
        }

        WiFi.setSleep(true);
    }
}

void loop()
{
    updateBatteryStatus();
    updateIMUReadings();

    leftDial.setValue(100 - leftEncoder.readEncoder());
    rightDial.setValue(100 - rightEncoder.readEncoder());

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

    // if left shoulder button is pressed, set left dial label to "Speed"
    if (mcp.digitalRead(pins::LEFT_SHOULDER_BTN) == LOW)
    {
        leftDial.setTextAndValue("Vibe", 100 - leftEncoder.readEncoder());
    }
    else
    {
        leftDial.setTextAndValue("Speed", 100 - leftEncoder.readEncoder());
    }

    // if left shoulder button is pressed, set left dial label to "Speed"
    if (mcp.digitalRead(pins::RIGHT_SHOULDER_BTN) == LOW)
    {
        rightDial.setTextAndValue("Stroke", 100 - rightEncoder.readEncoder());
    }
    else
    {
        rightDial.setTextAndValue("Depth", 100 - rightEncoder.readEncoder());
    }

    // every 2 seconds run the vibrator
    if (millis() - lastVibratorTime >= 2000)
    {
        lastVibratorTime = millis();
        playVibratorPattern(VibratorPattern::TRIPLE_PULSE);
        playBuzzerPattern(BuzzerPattern::SINGLE_BEEP);
    }
}
