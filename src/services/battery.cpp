#include "battery.h"

#include "CircularBuffer.hpp"
#include "esp_log.h"

static const char *TAG = "Battery";

// Initialize the global service instance
Adafruit_MAX17048 batteryService;

// Initialize battery status variables

CircularBuffer<float, 10> voltageBuffer;
uint8_t voltBatteryPercent = 0;
float voltBatteryVoltage = 0.0;

uint8_t getBatteryPercent() { return voltBatteryPercent; }

float getBatteryVoltage() { return voltBatteryVoltage; }

bool initBattery() {
    // Initialize the MAX17048 service
    if (!batteryService.begin()) {
        ESP_LOGE(TAG, "Failed to initialize MAX17048 battery service!");
        return false;
    }
    return true;
}

bool isCharging() {
    // If the buffer is not full, can't determine slope
    if (!voltageBuffer.isFull()) return false;

    // Compute the slope of the voltage over the buffer
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumXX = 0.0f;
    int n = voltageBuffer.size();
    for (int i = 0; i < n; ++i) {
        float x = (float)i;
        float y = voltageBuffer[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }
    float denominator = n * sumXX - sumX * sumX;
    float slope = 0.0f;
    if (denominator != 0.0f) {
        slope = (n * sumXY - sumX * sumY) / denominator;
    }

    // If the voltage is increasing, we are charging
    return slope > 0.0f;
}

void updateBatteryStatus() {
    // Single sensor read per field per tick
    voltBatteryPercent = (uint8_t)batteryService.cellPercent();

    voltBatteryVoltage = batteryService.cellVoltage();
    voltageBuffer.push(voltBatteryVoltage);

    ESP_LOGV(TAG,
             "Battery Update - Percent: %d%%, Voltage: %.2fV, Charge Rate: "
             "%.2f%%/hr",
             voltBatteryPercent, voltBatteryVoltage, lastChargeRate);
}