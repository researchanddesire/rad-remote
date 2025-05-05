#include "battery.h"

// Initialize the global service instance
Adafruit_MAX17048 batteryService;

bool initBatteryService()
{
    if (!batteryService.begin())
    {
        Serial.println("Error: MAX17048 not found!");
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