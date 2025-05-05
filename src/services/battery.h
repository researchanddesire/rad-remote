#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include <Adafruit_MAX1704X.h>

// Declare the global service instance
extern Adafruit_MAX17048 batteryService;

// Function declarations
bool initBatteryService();
uint8_t getBatteryPercent();
float getBatteryVoltage();
float getChargeRate();

#endif // BATTERY_H