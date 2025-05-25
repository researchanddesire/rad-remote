#include "battery.h"
#include "esp_log.h"

static const char *TAG = "Battery";

// Initialize the global service instance
Adafruit_MAX17048 batteryService;

// Initialize battery status variables
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
uint16_t fullChargeCapacity = 0;
uint16_t lastFullChargeCapacity = 0;

bool initBattery(uint16_t fullChargeCapacity)
{
    // Initialize the MAX17048 service
    if (!initBatteryService())
    {
        ESP_LOGD(TAG, "Failed to initialize MAX17048 battery service!");
        return false;
    }

    // Set the full charge capacity
    setFullChargeCapacity(fullChargeCapacity);

    return true;
}

bool initBatteryService()
{
    if (!batteryService.begin())
    {
        ESP_LOGD(TAG, "Error: MAX17048 not found!");
        return false;
    }
    return true;
}

uint8_t getBatteryPercent()
{
    return (uint8_t)batteryService.cellPercent();
}

float getBatteryVoltage()
{
    return batteryService.cellVoltage();
}

float getChargeRate()
{
    return batteryService.chargeRate();
}

uint16_t readJouleBatteryStatus()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00);            // CONTROL_STATUS command (0x00)
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
        ESP_LOGD(TAG, "Failed to read CONTROL_STATUS");
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

    ESP_LOGD(TAG, "BQ27220 unsealed");
}

void enterConfigUpdateMode()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00); // Control register
    Wire.write(0x13); // Enter config update mode command
    Wire.write(0x00); // Subcommand
    Wire.endTransmission();

    ESP_LOGD(TAG, "Entered config update mode");
}

void setDataFlashAddress(uint16_t address)
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00);                  // Control register
    Wire.write(0x61);                  // Data flash address command
    Wire.write(address & 0xFF);        // Address low byte
    Wire.write((address >> 8) & 0xFF); // Address high byte
    Wire.endTransmission();

    ESP_LOGD(TAG, "Set data flash address to 0x%04X", address);
}

void writeDataFlash(uint16_t data)
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x40);               // Data flash data register
    Wire.write(data & 0xFF);        // Data low byte
    Wire.write((data >> 8) & 0xFF); // Data high byte
    Wire.endTransmission();

    ESP_LOGD(TAG, "Wrote 0x%04X to data flash", data);
}

void updateChecksum()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00); // Control register
    Wire.write(0x60); // Update checksum command
    Wire.write(0x00); // Subcommand
    Wire.endTransmission();

    ESP_LOGD(TAG, "Updated checksum");
}

void exitConfigUpdateMode()
{
    Wire.beginTransmission(BQ27220_ADDR);
    Wire.write(0x00); // Control register
    Wire.write(0x42); // Exit config update mode command
    Wire.write(0x00); // Subcommand
    Wire.endTransmission();

    ESP_LOGD(TAG, "Exited config update mode");
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

    ESP_LOGD(TAG, "Set design capacity to %d mAh", capacity);
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

    ESP_LOGD(TAG, "Set full charge capacity to %d mAh", capacity);
}

void updateBatteryStatus()
{
    if (millis() - lastBatteryCheck >= 1000)
    { // Check every second
        lastBatteryCheck = millis();
        jouleBatteryPercent = readJouleBatteryPercent();
        jouleBatteryCurrent = readJouleBatteryCurrent();
        voltBatteryPercent = getBatteryPercent();
        voltBatteryVoltage = getBatteryVoltage();
        float currentChargeRate = getChargeRate();
        ESP_LOGD(TAG, "Battery Update - Joule: %d%%, Current: %d mA, Volt: %d%%, Voltage: %.2fV, Charge Rate: %.2f%%/hr",
                 jouleBatteryPercent, jouleBatteryCurrent, voltBatteryPercent, voltBatteryVoltage, currentChargeRate);
    }
}