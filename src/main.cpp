#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>
#include <AiEsp32RotaryEncoder.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/timer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Adafruit_MAX1704X.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include "SparkFunLSM6DS3.h"
#include "pins.h"
#include "constants.h"
#include "components/TextButton.h"
#include "services/display.h"
#include "services/mcp.h"
#include "services/leds.h"
#include "components/EncoderDial.h"

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

// BQ27220 I2C address
#define BQ27220_ADDR 0x55

// BQ27220 registers
#define BQ27220_SOC 0x2C   // State of Charge register
#define BQ27220_FLAGS 0x0A // Flags register

// MAX17048 setup
Adafruit_MAX17048 maxlipo;

// Add to control variables section
uint8_t jouleBatteryPercent = 0;
uint16_t jouleBatteryStatus = 0;
uint8_t voltBatteryPercent = 0;
float voltBatteryVoltage = 0.0;
float lastChargeRate = 0.0;
int16_t jouleBatteryCurrent = 0;
uint8_t lastJouleBatteryPercent = 0;
uint8_t lastVoltBatteryPercent = 0;
float lastVoltBatteryVoltage = 0.0;
int16_t lastJouleBatteryCurrent = 0;
unsigned long lastBatteryCheck = 0;

// Add to control variables section
bool enableSleep = true; // Flag to control sleep functionality

// LSM6DS3TR-C setup
LSM6DS3 imu(0x6A); // Initialize with address 0x6A
float accelX = 0, accelY = 0, accelZ = 0;
float lastAccelX = 0, lastAccelY = 0, lastAccelZ = 0;

// Add to control variables section
uint16_t fullChargeCapacity = 0;
uint16_t lastFullChargeCapacity = 0;

uint16_t readJouleBatteryStatus()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00);            // CONTROL_STATUS command (0x00)
                                 //   Wire.write(0x00);  // Subcommand MSB
                                 //   Wire.write(0x00);  // Subcommand LSB
    Wire.endTransmission(false); // Restart for read

    Wire.requestFrom(BQ27220_ADDR, 2);
    if (Wire.available() == 2)
    {
        uint8_t lowByte = Wire.read();
        uint8_t highByte = Wire.read();
        return (highByte << 8) | lowByte;
    }
    else
    {
        Serial.println("Failed to read CONTROL_STATUS");
        return 0xFFFF;
    }
}

uint8_t readJouleBatteryPercent()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(BQ27220_SOC);
    Wire.endTransmission(false);
    Wire.requestFrom(BQ27220_ADDR, 2);
    if (Wire.available() == 2)
    {
        uint8_t lowByte = Wire.read();
        uint8_t highByte = Wire.read();
        // Serial.printf("Joule Battery high byte: %d, low byte: %d\n", highByte, lowByte);
        uint16_t socRaw = (highByte << 8) | lowByte;

        return socRaw; // SOC is typically returned in 0.01% units
    }
    return 1;
}

int16_t readJouleBatteryCurrent()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x0C); // Current register
    Wire.endTransmission(false);
    Wire.requestFrom(BQ27220_ADDR, 2);
    if (Wire.available() == 2)
    {
        uint8_t lowByte = Wire.read();
        uint8_t highByte = Wire.read();
        int16_t currentRaw = (highByte << 8) | lowByte;
        return currentRaw; // Returns current in mA
    }
    return 0;
}

uint16_t readFullChargeCapacity()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x12); // Full Charge Capacity register
    Wire.endTransmission(false);
    Wire.requestFrom(BQ27220_ADDR, 2);
    if (Wire.available() == 2)
    {
        uint8_t lowByte = Wire.read();
        uint8_t highByte = Wire.read();
        return (highByte << 8) | lowByte; // Returns capacity in mAh
    }
    return 0;
}

// Control variables
uint8_t rainbowSpeed = 20; // Initial speed (ms delay)
unsigned long vibratorStartTime = 0;
bool vibratorActive = false;
unsigned long lastDebugTime = 0;
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;
static unsigned long lastToggle = 0;

// Rotary encoder instances
AiEsp32RotaryEncoder leftEncoder = AiEsp32RotaryEncoder(pins::LEFT_ENCODER_A, pins::LEFT_ENCODER_B, -1, -1);
AiEsp32RotaryEncoder rightEncoder = AiEsp32RotaryEncoder(pins::RIGHT_ENCODER_A, pins::RIGHT_ENCODER_B, -1, -1);

void IRAM_ATTR readLeftEncoder()
{
    leftEncoder.readEncoder_ISR();
}

void IRAM_ATTR readRightEncoder()
{
    rightEncoder.readEncoder_ISR();
}

// // Button handling task
// void buttonTask(void *parameter)
// {
//     // Check if center button is actually pressed (debounce)
//     if (mcp.digitalRead(pins::CENTER_BTN) == LOW)
//     {
//         // Activate vibrator
//         vibratorActive = true;
//         vibratorStartTime = millis();

//         // Activate buzzer
//         buzzerActive = true;
//         buzzerStartTime = millis();
//     }
//     vTaskDelete(NULL); // Delete this task when done
// }

// Timer ISR for buzzer PWM
void IRAM_ATTR buzzerTimerISR(void *arg)
{
    if (buzzerActive)
    {
        mcp.digitalWrite(pins::BUZZER, !mcp.digitalRead(pins::BUZZER));
    }
}

void updateBatteryStatus()
{
    if (millis() - lastBatteryCheck >= 1000)
    { // Check every second
        lastBatteryCheck = millis();
        jouleBatteryPercent = readJouleBatteryPercent();
        jouleBatteryCurrent = readJouleBatteryCurrent();
        voltBatteryPercent = (uint8_t)maxlipo.cellPercent();
        voltBatteryVoltage = maxlipo.cellVoltage();
        float currentChargeRate = maxlipo.chargeRate();
        Serial.printf("Battery Update - Joule: %d%%, Current: %d mA, Volt: %d%%, Voltage: %.2fV, Charge Rate: %.2f%%/hr\n",
                      jouleBatteryPercent, jouleBatteryCurrent, voltBatteryPercent, voltBatteryVoltage, currentChargeRate);
    }
}

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

void unsealBQ27220()
{
    // Send unseal key (0xFFFFFF)
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00); // Control register
    Wire.write(0xFF); // Key MSB
    Wire.write(0xFF); // Key middle byte
    Wire.endTransmission();

    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00); // Control register
    Wire.write(0xFF); // Key LSB
    Wire.write(0xFF); // Key MSB
    Wire.endTransmission();

    Serial.println("BQ27220 unsealed");
}

void enterConfigUpdateMode()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00); // Control register
    Wire.write(0x13); // Enter config update mode command
    Wire.write(0x00); // Subcommand
    Wire.endTransmission();

    Serial.println("Entered config update mode");
}

void setDataFlashAddress(uint16_t address)
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00);                  // Control register
    Wire.write(0x61);                  // Data flash address command
    Wire.write(address & 0xFF);        // Address low byte
    Wire.write((address >> 8) & 0xFF); // Address high byte
    Wire.endTransmission();

    Serial.printf("Set data flash address to 0x%04X\n", address);
}

void writeDataFlash(uint16_t data)
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x40);               // Data flash data register
    Wire.write(data & 0xFF);        // Data low byte
    Wire.write((data >> 8) & 0xFF); // Data high byte
    Wire.endTransmission();

    Serial.printf("Wrote 0x%04X to data flash\n", data);
}

void updateChecksum()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00); // Control register
    Wire.write(0x60); // Update checksum command
    Wire.write(0x00); // Subcommand
    Wire.endTransmission();

    Serial.println("Updated checksum");
}

void exitConfigUpdateMode()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00); // Control register
    Wire.write(0x42); // Exit config update mode command
    Wire.write(0x00); // Subcommand
    Wire.endTransmission();

    Serial.println("Exited config update mode");
}

void writeDesignCapacity(uint16_t capacity)
{
    // Set data flash address to Design Capacity (0x0C)
    setDataFlashAddress(0x0C);
    delay(100);

    // Write the new capacity value
    writeDataFlash(capacity);
    delay(100);

    // Update checksum
    updateChecksum();
    delay(100);

    Serial.printf("Set design capacity to %d mAh\n", capacity);
}

void setFullChargeCapacity(uint16_t capacity)
{
    // Unseal the device
    unsealBQ27220();
    delay(100);

    // Enter config update mode
    enterConfigUpdateMode();
    delay(100);

    // Set data flash address to FCC (0x0A)
    setDataFlashAddress(0x0A);
    delay(100);

    // Write the new capacity value
    writeDataFlash(capacity);
    delay(100);

    // Update checksum
    updateChecksum();
    delay(100);

    // Also set the design capacity
    writeDesignCapacity(capacity);

    // Exit config update mode
    exitConfigUpdateMode();

    Serial.printf("Set full charge capacity to %d mAh\n", capacity);
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

    // Initialize LSM6DS3TR-C
    if (!imu.begin())
    {
        Serial.println("Failed to find LSM6DS3TR-C chip");
        while (1)
        {
            delay(10);
        }
    }
    else
    {
        Serial.println("LSM6DS3TR-C Found!");
    }
    // // Configure accelerometer
    // imu.settings.accelEnabled = 1;
    // imu.settings.accelRange = 2;  // 2G range
    // imu.settings.accelSampleRate = 104;  // 104Hz
    // imu.settings.accelBandWidth = 10;  // 10Hz bandwidth

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

    // Configure buzzer timer
    timer_config_t timerConfig = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .divider = 80 // 80MHz / 80 = 1MHz timer frequency
    };

    // Initialize encoders
    leftEncoder.begin();
    rightEncoder.begin();

    leftEncoder.setup(readLeftEncoder);
    rightEncoder.setup(readRightEncoder);

    // Set encoder boundaries and step size
    leftEncoder.setBoundaries(0, 100, false);  // 0-100% brightness
    leftEncoder.setAcceleration(0);            // No acceleration for linear response
    rightEncoder.setBoundaries(1, 100, false); // 1-100ms delay
    rightEncoder.setAcceleration(0);           // No acceleration for linear response

    // Set initial values
    leftEncoder.setEncoderValue(brightness * 100 / 255); // Convert 0-255 to 0-100
    rightEncoder.setEncoderValue(rainbowSpeed);

    // Initialize battery monitors
    Wire.beginTransmission(BQ27220_ADDR);
    if (Wire.endTransmission() != 0)
    {
        Serial.println("Error: BQ27220 not found!");
    }

    if (!maxlipo.begin())
    {
        Serial.println("Error: MAX17048 not found!");
    }

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
}

void handleVibrator()
{
    if (vibratorActive && (millis() - vibratorStartTime >= 1000))
    {
        vibratorActive = false;
        mcp.digitalWrite(pins::VIBRATOR, LOW);
    }
    else if (vibratorActive && (millis() - vibratorStartTime < 1000))
    {
        mcp.digitalWrite(pins::VIBRATOR, HIGH);
    }
}

void handleBuzzer()
{
    if (buzzerActive)
    {
        buzzerStartTime = millis();
        while (millis() - buzzerStartTime <= 500)
        {
            buzzerActive = false;
            // Toggle buzzer every 1ms

            mcp.digitalWrite(pins::BUZZER, HIGH);
            delay(1);
            mcp.digitalWrite(pins::BUZZER, LOW);
        }
        mcp.digitalWrite(pins::BUZZER, LOW);
    }
}

void drawShoulderButtons()
{

    int buttonWidth = 60;
    int buttonHeight = 25;
    int buttonRadius = 5;

    // tft.drawRoundRect(0, 0, buttonWidth, buttonHeight, buttonRadius, 0x7BEF);
    tft.drawRoundRect(DISPLAY_WIDTH - buttonWidth, 0, buttonWidth, buttonHeight, buttonRadius, 0x7BEF);
    tft.setTextColor(0x7BEF);

    // // Center "left" text in left button
    // tft.setTextSize(1);
    int16_t textWidth = 4 * 6; // "left" is 4 characters, approx 6 pixels per character
    int16_t textX = (buttonWidth - textWidth) / 2;
    int16_t textY = (buttonHeight - 8) / 2; // 8 is approx height of text
    // tft.setCursor(textX, textY);
    // tft.print("back up");

    // Center "right" text in right button
    textWidth = 5 * 6; // "right" is 5 characters
    textX = DISPLAY_WIDTH - buttonWidth + (buttonWidth - textWidth) / 2;
    tft.setCursor(textX, textY);
    tft.print("deeper");
}

void drawBottomButtons()
{
    int buttonWidth = 60;
    int buttonHeight = 25;
    int buttonRadius = 5;

    // bottom left and bottom right
    tft.drawRoundRect(0, DISPLAY_HEIGHT - buttonHeight, buttonWidth, buttonHeight, buttonRadius, 0x7BEF);
    tft.drawRoundRect(DISPLAY_WIDTH - buttonWidth, DISPLAY_HEIGHT - buttonHeight, buttonWidth, buttonHeight, buttonRadius, 0x7BEF);

    // text - centered in buttons
    tft.setTextColor(0x7BEF);
    tft.setTextSize(1);

    // Center "left" text in bottom left button
    int16_t leftTextWidth = 4 * 6; // "left" is 4 characters, approx 6 pixels per character
    int16_t leftTextX = (buttonWidth - leftTextWidth) / 2;
    int16_t textY = (buttonHeight - 8) / 2; // 8 is approx height of text
    tft.setCursor(leftTextX, DISPLAY_HEIGHT - buttonHeight + textY);
    tft.print("left");

    // Center "right" text in bottom right button
    int16_t rightTextWidth = 5 * 6; // "right" is 5 characters
    int16_t rightTextX = DISPLAY_WIDTH - buttonWidth + (buttonWidth - rightTextWidth) / 2;
    tft.setCursor(rightTextX, DISPLAY_HEIGHT - buttonHeight + textY);
    tft.print("right");

    // and two small icon buttons, also rounded rect but near the center of the screen
    // Calculate positions to center the two small buttons
    int centerSpacing = 20;             // Space between the two center buttons
    int smallButtonSize = buttonHeight; // Using buttonHeight for square buttons
    int leftCenterX = DISPLAY_WIDTH / 2 - smallButtonSize - centerSpacing / 2;
    int rightCenterX = DISPLAY_WIDTH / 2 + centerSpacing / 2;

    // Draw the two centered buttons
    tft.drawRoundRect(leftCenterX, DISPLAY_HEIGHT - buttonHeight, smallButtonSize, smallButtonSize, buttonRadius, 0x7BEF);
    tft.drawRoundRect(rightCenterX, DISPLAY_HEIGHT - buttonHeight, smallButtonSize, smallButtonSize, buttonRadius, 0x7BEF);
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

        // Configure wake sources
        // esp_sleep_enable_timer_wakeup(1000 * 1000); // Wake up 450ms later (50ms before next transmission)
        WiFi.setSleep(true);
        // Save any necessary state before sleep
        // Note: Most peripherals will need to be reinitialized after wake

        // Enter light sleep
        // Enable wake on encoder pins
        // esp_sleep_enable_ext1_wakeup(BIT(32), ESP_EXT1_WAKEUP_ANY_HIGH); // Left encoder A
        // esp_sleep_enable_ext1_wakeup(BIT(33), ESP_EXT1_WAKEUP_ANY_HIGH); // Left encoder B
        // esp_sleep_enable_ext1_wakeup(BIT(34), ESP_EXT1_WAKEUP_ANY_HIGH); // Right encoder A
        // esp_sleep_enable_ext1_wakeup(BIT(35), ESP_EXT1_WAKEUP_ANY_HIGH); // Right encoder B

        // // Enable wake on MCP23017 interrupt pins
        // esp_sleep_enable_ext1_wakeup(BIT(16), ESP_EXT1_WAKEUP_ANY_HIGH); // MCP GPIOA
        // esp_sleep_enable_ext1_wakeup(BIT(17), ESP_EXT1_WAKEUP_ANY_HIGH); // MCP GPIOB

        // esp_light_sleep_start();

        // Code after this point will not be executed until after wake
    }
}

// Add function to toggle sleep functionality
void toggleSleep()
{
    enableSleep = !enableSleep;
    Serial.printf("Sleep mode %s\n", enableSleep ? "enabled" : "disabled");
}

void loop()
{
    handleVibrator();
    handleBuzzer();
    updateBatteryStatus();
    // broadcastEspNow(); // Add ESP-NOW broadcasting

    // // Show rainbow effect with current speed
    // rainbow(rainbowSpeed);

    leftDial.setValue(100 - leftEncoder.readEncoder());
    rightDial.setValue(100 - rightEncoder.readEncoder());

    // Print debug info every 2 seconds
    if (millis() - lastDebugTime >= 50)
    {
        // updateScreen();

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

    // delay(1); // Small delay to prevent overwhelming the system
}
