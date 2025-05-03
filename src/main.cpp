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

// LED strip setup
#define LED_PIN 23
#define NUM_LEDS 3
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Screen pins
#define TFT_BACKLIGHT 12
#define TFT_RST 13
#define TFT_A0 14
#define TFT_CS 25
#define TFT_SCK 26
#define TFT_SDA 27

// Screen setup
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_A0, TFT_RST);

// Function declarations
uint32_t Wheel(byte WheelPos)
{
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170)
    {
        WheelPos -= 85;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

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
        Serial.printf("Joule Battery high byte: %d, low byte: %d\n", highByte, lowByte);
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

// Non-blocking rainbow function
void rainbow(uint8_t wait)
{
    if (millis() - lastRainbowUpdate >= wait)
    {
        lastRainbowUpdate = millis();

        // Update all pixels
        for (int i = 0; i < strip.numPixels(); i++)
        {
            strip.setPixelColor(i, Wheel((i + rainbowIndex) & 255));
        }
        strip.show();

        // Increment rainbow index
        rainbowIndex = (rainbowIndex + 1) & 255;
    }
}

// MCP23017 setup
Adafruit_MCP23X17 mcp;

// Encoder pins
#define LEFT_ENCODER_A 34
#define LEFT_ENCODER_B 35
#define RIGHT_ENCODER_A 18
#define RIGHT_ENCODER_B 19

// MCP23017 pin definitions
#define RIGHT_SHOULDER_BTN 0
#define BUZZER 1
#define GYRO_INT1 3
#define GYRO_INT2 4
#define LEFT_SHOULDER_BTN 5
#define VIBRATOR 6
#define REGULATOR_EN 7
#define FUEL_GAUGE 8
#define EXT_IO3 9
#define EXT_IO4 10
#define LEFT_BTN 12
#define CENTER_BTN 13
#define RIGHT_BTN 14

// MCP23017 interrupt pins
#define MCP_INTB_PIN 16 // GPIOB interrupt
#define MCP_INTA_PIN 17 // GPIOA interrupt

// Control variables
uint8_t brightness = 128;  // Initial brightness (0-255)
uint8_t rainbowSpeed = 20; // Initial speed (ms delay)
unsigned long vibratorStartTime = 0;
bool vibratorActive = false;
unsigned long lastDebugTime = 0;
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;
static unsigned long lastToggle = 0;

// Rotary encoder instances
AiEsp32RotaryEncoder leftEncoder = AiEsp32RotaryEncoder(LEFT_ENCODER_A, LEFT_ENCODER_B, -1, -1);
AiEsp32RotaryEncoder rightEncoder = AiEsp32RotaryEncoder(RIGHT_ENCODER_A, RIGHT_ENCODER_B, -1, -1);

void IRAM_ATTR readLeftEncoder()
{
    leftEncoder.readEncoder_ISR();
}

void IRAM_ATTR readRightEncoder()
{
    rightEncoder.readEncoder_ISR();
}
// Button handling task
void buttonTask(void *parameter)
{
    // Read all button states
    bool rightShoulder = (mcp.digitalRead(RIGHT_SHOULDER_BTN) == LOW);
    bool leftShoulder = (mcp.digitalRead(LEFT_SHOULDER_BTN) == LOW);
    bool leftBtn = (mcp.digitalRead(LEFT_BTN) == LOW);
    bool centerBtn = (mcp.digitalRead(CENTER_BTN) == LOW);
    bool rightBtn = (mcp.digitalRead(RIGHT_BTN) == LOW);
    bool extIO3 = (mcp.digitalRead(EXT_IO3) == LOW);
    bool extIO4 = (mcp.digitalRead(EXT_IO4) == LOW);
    bool gyroInt1 = (mcp.digitalRead(GYRO_INT1) == LOW);
    bool gyroInt2 = (mcp.digitalRead(GYRO_INT2) == LOW);

    // Handle center button press
    if (centerBtn)
    {
        // Activate vibrator
        vibratorActive = true;
        vibratorStartTime = millis();

        // Activate buzzer
        buzzerActive = true;
        buzzerStartTime = millis();
    }

    // Update last button states
    lastRightShoulder = rightShoulder;
    lastLeftShoulder = leftShoulder;
    lastLeftBtn = leftBtn;
    lastCenterBtn = centerBtn;
    lastRightBtn = rightBtn;

    vTaskDelete(NULL); // Delete this task when done
}
// MCP23017 interrupt handler - creates a task to handle the button press
void IRAM_ATTR handleMCPInterruptA()
{
    TaskHandle_t buttonTaskHandle = NULL;
    xTaskCreatePinnedToCore(
        buttonTask,        // Task function
        "ButtonTaskA",     // Task name
        2048,              // Stack size
        NULL,              // Task parameters
        1,                 // Task priority
        &buttonTaskHandle, // Task handle
        1                  // Core to run on (core 1)
    );
}

void IRAM_ATTR handleMCPInterruptB()
{
    TaskHandle_t buttonTaskHandle = NULL;
    xTaskCreatePinnedToCore(
        buttonTask,        // Task function
        "ButtonTaskB",     // Task name
        2048,              // Stack size
        NULL,              // Task parameters
        1,                 // Task priority
        &buttonTaskHandle, // Task handle
        1                  // Core to run on (core 1)
    );
}

// Timer ISR for buzzer PWM
void IRAM_ATTR buzzerTimerISR(void *arg)
{
    if (buzzerActive)
    {
        mcp.digitalWrite(BUZZER, !mcp.digitalRead(BUZZER));
    }
}

void printDebugInfo()
{
    Serial.println("\n=== IO Status ===");
    Serial.println("ESP32 Local IO:");
    Serial.printf("Left Encoder A: %d\n", digitalRead(LEFT_ENCODER_A));
    Serial.printf("Left Encoder B: %d\n", digitalRead(LEFT_ENCODER_B));
    Serial.printf("Right Encoder A: %d\n", digitalRead(RIGHT_ENCODER_A));
    Serial.printf("Right Encoder B: %d\n", digitalRead(RIGHT_ENCODER_B));
    Serial.printf("LED Pin: %d\n", digitalRead(LED_PIN));

    Serial.println("\nMCP23017 Remote IO:");
    Serial.printf("Right Shoulder Button: %d\n", mcp.digitalRead(RIGHT_SHOULDER_BTN));
    Serial.printf("Buzzer: %d\n", mcp.digitalRead(BUZZER));
    Serial.printf("Gyro INT1: %d\n", mcp.digitalRead(GYRO_INT1));
    Serial.printf("Gyro INT2: %d\n", mcp.digitalRead(GYRO_INT2));
    Serial.printf("Left Shoulder Button: %d\n", mcp.digitalRead(LEFT_SHOULDER_BTN));
    Serial.printf("Vibrator: %d\n", mcp.digitalRead(VIBRATOR));
    Serial.printf("Regulator Enable: %d\n", mcp.digitalRead(REGULATOR_EN));
    Serial.printf("Fuel Gauge: %d\n", mcp.digitalRead(FUEL_GAUGE));
    Serial.printf("Ext. IO3: %d\n", mcp.digitalRead(EXT_IO3));
    Serial.printf("Ext. IO4: %d\n", mcp.digitalRead(EXT_IO4));
    Serial.printf("Left Button: %d\n", mcp.digitalRead(LEFT_BTN));
    Serial.printf("Center Button: %d\n", mcp.digitalRead(CENTER_BTN));
    Serial.printf("Right Button: %d\n", mcp.digitalRead(RIGHT_BTN));

    Serial.println("\nAccelerometer:");
    Serial.printf("X: %.2f m/s^2\n", accelX);
    Serial.printf("Y: %.2f m/s^2\n", accelY);
    Serial.printf("Z: %.2f m/s^2\n", accelZ);

    Serial.println("\nBattery Status:");
    jouleBatteryStatus = readJouleBatteryStatus();
    Serial.printf("Joule Battery Status: %d%%\n", jouleBatteryStatus);
    Serial.printf("Joule Battery: %d%%\n", jouleBatteryPercent);
    Serial.printf("Volt Battery: %d%%\n", voltBatteryPercent);

    Serial.println("\nControl Variables:");
    Serial.printf("Brightness: %d\n", brightness);
    Serial.printf("Rainbow Speed: %d\n", rainbowSpeed);
    Serial.printf("Vibrator Active: %s\n", vibratorActive ? "Yes" : "No");
    if (vibratorActive)
    {
        Serial.printf("Vibrator Time Remaining: %lu ms\n", 1000 - (millis() - vibratorStartTime));
    }
    Serial.printf("Buzzer Active: %s\n", buzzerActive ? "Yes" : "No");
    if (buzzerActive)
    {
        Serial.printf("Buzzer Time Remaining: %lu ms\n", 500 - (millis() - buzzerStartTime));
    }
    Serial.println("================\n");
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
    Wire.begin(21, 22); // SDA, SCL

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

    // // Apply the settings
    // imu.begin();

    // Initialize screen
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, HIGH);
    SPI.begin(TFT_SCK, -1, TFT_SDA, -1);
    tft.init(240, 320); // Initialize with screen dimensions
    tft.setRotation(1); // Landscape mode
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);

    // Initialize MCP23017
    if (!mcp.begin_I2C())
    {
        Serial.println("Error: MCP23017 not found!");
        while (1)
            ;
    }

    // Configure MCP23017 pins
    // Input pins with pull-up
    mcp.pinMode(RIGHT_SHOULDER_BTN, INPUT_PULLUP);
    mcp.pinMode(LEFT_SHOULDER_BTN, INPUT_PULLUP);
    mcp.pinMode(LEFT_BTN, INPUT_PULLUP);
    mcp.pinMode(CENTER_BTN, INPUT_PULLUP);
    mcp.pinMode(RIGHT_BTN, INPUT_PULLUP);
    mcp.pinMode(EXT_IO3, INPUT_PULLUP);
    mcp.pinMode(EXT_IO4, INPUT_PULLUP);
    mcp.pinMode(GYRO_INT1, INPUT_PULLUP);
    mcp.pinMode(GYRO_INT2, INPUT_PULLUP);

    // Output pins
    mcp.pinMode(BUZZER, OUTPUT);
    mcp.pinMode(VIBRATOR, OUTPUT);
    mcp.pinMode(REGULATOR_EN, OUTPUT);
    mcp.pinMode(FUEL_GAUGE, OUTPUT);

    // Configure MCP23017 interrupts
    mcp.setupInterrupts(true, false, LOW); // Enable interrupts, mirror INTA/B, active LOW

    // Configure interrupt pins
    pinMode(MCP_INTA_PIN, INPUT_PULLUP);
    pinMode(MCP_INTB_PIN, INPUT_PULLUP);

    // Set up interrupts for all buttons
    mcp.setupInterruptPin(RIGHT_SHOULDER_BTN, CHANGE);
    mcp.setupInterruptPin(LEFT_SHOULDER_BTN, CHANGE);
    mcp.setupInterruptPin(LEFT_BTN, CHANGE);
    mcp.setupInterruptPin(CENTER_BTN, CHANGE);
    mcp.setupInterruptPin(RIGHT_BTN, CHANGE);
    mcp.setupInterruptPin(EXT_IO3, CHANGE);
    mcp.setupInterruptPin(EXT_IO4, CHANGE);
    mcp.setupInterruptPin(GYRO_INT1, CHANGE);
    mcp.setupInterruptPin(GYRO_INT2, CHANGE);

    // Attach interrupt handlers
    attachInterrupt(digitalPinToInterrupt(MCP_INTA_PIN), handleMCPInterruptA, FALLING);
    attachInterrupt(digitalPinToInterrupt(MCP_INTB_PIN), handleMCPInterruptB, FALLING);

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

    // Initialize LED strip
    strip.begin();
    strip.setBrightness(brightness);
    strip.show(); // Initialize all pixels to 'off'

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

void handleEncoders()
{
    // Update brightness from left encoder (0-100%)
    if (leftEncoder.encoderChanged())
    {
        uint8_t percent = 100 - leftEncoder.readEncoder(); // Invert direction
        brightness = (percent * 255) / 100;                // Convert 0-100 to 0-255
        strip.setBrightness(brightness);
        strip.show();
    }

    // Update speed from right encoder (1-100ms)
    if (rightEncoder.encoderChanged())
    {
        rainbowSpeed = 100 - rightEncoder.readEncoder(); // Invert direction
    }
}

void handleVibrator()
{
    if (vibratorActive && (millis() - vibratorStartTime >= 1000))
    {
        vibratorActive = false;
        mcp.digitalWrite(VIBRATOR, LOW);
    }
    else if (vibratorActive && (millis() - vibratorStartTime < 1000))
    {
        mcp.digitalWrite(VIBRATOR, HIGH);
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

            mcp.digitalWrite(BUZZER, HIGH);
            delay(1);
            mcp.digitalWrite(BUZZER, LOW);
        }
        mcp.digitalWrite(BUZZER, LOW);
    }
}

void drawProgressBar(int x, int y, int width, int height, int value, int maxValue, uint16_t color)
{
    // Draw background
    tft.fillRect(x, y, width, height, 0x7BEF); // Dark grey color
    // Draw progress from bottom to top
    int progress = (value * height) / maxValue;
    tft.fillRect(x, y + height - progress, width, progress, color);
    // Draw border
    tft.drawRect(x, y, width, height, ST77XX_WHITE);
}

void updateScreen()
{
    // Only clear the screen if we need to redraw everything
    static bool firstDraw = true;
    if (firstDraw)
    {
        tft.fillScreen(ST77XX_BLACK);
        firstDraw = false;
    }

    // Draw vertical progress bars for encoders
    int leftValue = 100 - leftEncoder.readEncoder();
    int rightValue = 100 - rightEncoder.readEncoder();

    // Left progress bar
    drawProgressBar(10, 40, 20, 180, leftValue, 100, ST77XX_MAGENTA);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(40, 40);
    tft.printf("Left: %d%%", leftValue);

    // Right progress bar
    drawProgressBar(290, 40, 20, 180, rightValue, 100, ST77XX_MAGENTA);
    tft.setCursor(40, 60);
    tft.printf("Right: %d%%", rightValue);

    // Read accelerometer data
    accelX = imu.readFloatAccelX();
    accelY = imu.readFloatAccelY();
    accelZ = imu.readFloatAccelZ();

    // Draw acceleration values
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(40, 100);
    tft.println("Acceleration");

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);

    // Update acceleration values if changed
    if (accelX != lastAccelX || accelY != lastAccelY || accelZ != lastAccelZ)
    {
        tft.fillRect(40, 120, 240, 60, ST77XX_BLACK);
        tft.setCursor(40, 120);
        tft.printf("X: %.2f m/s^2\n", accelX);
        tft.setCursor(40, 130);
        tft.printf("Y: %.2f m/s^2\n", accelY);
        tft.setCursor(40, 140);
        tft.printf("Z: %.2f m/s^2\n", accelZ);

        lastAccelX = accelX;
        lastAccelY = accelY;
        lastAccelZ = accelZ;
    }

    // Update battery status if changed
    if (jouleBatteryPercent != lastJouleBatteryPercent)
    {
        tft.fillRect(40, 180, 240, 10, ST77XX_BLACK);
        tft.setCursor(40, 180);
        tft.printf("Joule Batt: %d%%\n", jouleBatteryPercent);
        lastJouleBatteryPercent = jouleBatteryPercent;
    }

    if (voltBatteryPercent != lastVoltBatteryPercent ||
        voltBatteryVoltage != lastVoltBatteryVoltage ||
        maxlipo.chargeRate() != lastChargeRate ||
        jouleBatteryCurrent != lastJouleBatteryCurrent)
    {
        tft.fillRect(40, 190, 240, 40, ST77XX_BLACK);
        tft.setCursor(40, 190);
        tft.printf("Volt Batt: %d%%\n", voltBatteryPercent);
        tft.setCursor(40, 200);
        tft.printf("Voltage: %.2fV\n", voltBatteryVoltage);
        tft.setCursor(40, 210);
        tft.printf("Charge Rate: %.2f%%/hr\n", maxlipo.chargeRate());
        tft.setCursor(40, 220);
        tft.printf("Current: %d mA\n", jouleBatteryCurrent);
        lastVoltBatteryPercent = voltBatteryPercent;
        lastVoltBatteryVoltage = voltBatteryVoltage;
        lastChargeRate = maxlipo.chargeRate();
        lastJouleBatteryCurrent = jouleBatteryCurrent;
    }

    // Update full charge capacity if changed
    fullChargeCapacity = readFullChargeCapacity();
    if (fullChargeCapacity != lastFullChargeCapacity)
    {
        tft.fillRect(40, 230, 240, 20, ST77XX_BLACK);
        tft.setCursor(40, 230);
        tft.printf("Full Capacity: %d mAh\n", fullChargeCapacity);
        lastFullChargeCapacity = fullChargeCapacity;
    }

    // Only update values that have changed
    int16_t currentBrightness = (brightness * 100) / 255;
    if (currentBrightness != lastBrightness)
    {
        tft.fillRect(40, 240, 240, 20, ST77XX_BLACK);
        tft.setCursor(40, 250);
        tft.printf("Brightness: %d%%\n", currentBrightness);
        lastBrightness = currentBrightness;
    }

    if (rainbowSpeed != lastSpeed)
    {
        tft.fillRect(40, 250, 240, 20, ST77XX_BLACK);
        tft.setCursor(40, 260);
        tft.printf("Speed: %dms\n", rainbowSpeed);
        lastSpeed = rainbowSpeed;
    }

    if (vibratorActive != lastVibratorState)
    {
        tft.fillRect(40, 260, 240, 20, ST77XX_BLACK);
        tft.setCursor(40, 270);
        tft.printf("Vibrator: %s\n", vibratorActive ? "ON" : "OFF");
        lastVibratorState = vibratorActive;
    }

    if (buzzerActive != lastBuzzerState)
    {
        tft.fillRect(40, 270, 240, 20, ST77XX_BLACK);
        tft.setCursor(40, 280);
        tft.printf("Buzzer: %s\n", buzzerActive ? "ON" : "OFF");
        lastBuzzerState = buzzerActive;
    }

    // Update last IO values
    lastLeftEncoder = leftEncoder.readEncoder();
    lastRightEncoder = rightEncoder.readEncoder();
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

            // Enter deep sleep if enabled
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
    handleEncoders();
    handleVibrator();
    handleBuzzer();
    updateBatteryStatus();
    broadcastEspNow(); // Add ESP-NOW broadcasting

    // Show rainbow effect with current speed
    rainbow(rainbowSpeed);

    // Print debug info every 2 seconds
    if (millis() - lastDebugTime >= 500)
    {
        printDebugInfo();
        updateScreen();
        lastDebugTime = millis();
    }

    delay(1); // Small delay to prevent overwhelming the system
}
