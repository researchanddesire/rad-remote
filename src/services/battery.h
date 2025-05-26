#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include <Adafruit_MAX1704X.h>
#include <Wire.h>

// BQ27220 I2C address and registers
#define BQ27220_ADDR 0x55
#define BQ27220_SOC 0x2C   // State of Charge register
#define BQ27220_FLAGS 0x0A // Flags register

// Declare the global service instance
extern Adafruit_MAX17048 batteryService;

// Function declarations
bool initBatteryService();
bool initBattery(uint16_t fullChargeCapacity = 500); // Default to 500mAh if not specified

// MAX17048 functions
uint8_t getBatteryPercent();
float getBatteryVoltage();
float getChargeRate();

// BQ27220 functions
uint16_t readJouleBatteryStatus();
uint8_t readJouleBatteryPercent();
int16_t readJouleBatteryCurrent();
uint16_t readFullChargeCapacity();
void unsealBQ27220();
void enterConfigUpdateMode();
void setDataFlashAddress(uint16_t address);
void writeDataFlash(uint16_t data);
void updateChecksum();
void exitConfigUpdateMode();
void writeDesignCapacity(uint16_t capacity);
void setFullChargeCapacity(uint16_t capacity);

// Battery status variables
extern uint8_t jouleBatteryPercent;
extern uint16_t jouleBatteryStatus;
extern uint8_t voltBatteryPercent;
extern float voltBatteryVoltage;
extern float lastChargeRate;
extern int16_t jouleBatteryCurrent;
extern uint8_t lastJouleBatteryPercent;
extern uint8_t lastVoltBatteryPercent;
extern float lastVoltBatteryVoltage;
extern int16_t lastJouleBatteryCurrent;
extern unsigned long lastBatteryCheck;
extern uint16_t fullChargeCapacity;
extern uint16_t lastFullChargeCapacity;

// Battery update function
void updateBatteryStatus();

// Charging status function
bool isCharging();

#endif // BATTERY_H