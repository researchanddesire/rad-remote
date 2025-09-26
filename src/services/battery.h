#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

#include <Adafruit_MAX1704X.h>

// MAX17048 I2C address and registers
#define MAX17048_ADDR 0x36
#define MAX17048_VCELL_REG 0x02
#define MAX17048_SOC_REG 0x04
#define MAX17048_MODE_REG 0x06
#define MAX17048_VERSION_REG 0x08
#define MAX17048_CONFIG_REG 0x0C
#define MAX17048_COMMAND_REG 0xFE

// Declare the global service instance
extern Adafruit_MAX17048 batteryService;

// Function declarations
bool initBattery();  // Kept for API compatibility; capacity unused

// MAX17048 accessors
uint8_t getBatteryPercent();
float getBatteryVoltage();

// Battery status variables
extern uint8_t voltBatteryPercent;
extern float voltBatteryVoltage;
extern float lastChargeRate;
extern unsigned long lastBatteryCheck;

// Battery update function
void updateBatteryStatus();

// Charging status function
bool isCharging();

#endif  // BATTERY_H