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
float lastChargeRate = 0.0;

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
    return false;  // Disable charging detection.

    // The improved logic below fixes false positives while discharging, but
    // still gives false negatives when actually charging when at high battery
    // percent.
    // We have a hardware LED for charge status, so this is redundant anyway.

    // If the buffer is not full, can't determine slope
    if (!voltageBuffer.isFull()) return false;

    // Compute the slope of the voltage over the buffer
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumXX = 0.0f;
    int n = voltageBuffer.size();

    // Log all voltage readings for debugging
    ESP_LOGD(TAG, "Voltage buffer contents (size: %d):", n);
    for (int i = 0; i < n; ++i) {
        float x = (float)i;
        float y = voltageBuffer[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
        ESP_LOGD(TAG, "  [%d]: %.3fV", i, y);
    }

    float denominator = n * sumXX - sumX * sumX;
    float slope = 0.0f;
    if (denominator != 0.0f) {
        slope = (n * sumXY - sumX * sumY) / denominator;
    } else {
        ESP_LOGW(TAG,
                 "isCharging: Denominator is zero, cannot calculate slope");
        return false;
    }

    // Calculate additional metrics for better analysis
    float minVoltage = voltageBuffer[0];
    float maxVoltage = voltageBuffer[0];
    float avgVoltage = sumY / n;

    for (int i = 1; i < n; ++i) {
        if (voltageBuffer[i] < minVoltage) minVoltage = voltageBuffer[i];
        if (voltageBuffer[i] > maxVoltage) maxVoltage = voltageBuffer[i];
    }

    float voltageRange = maxVoltage - minVoltage;

    // Define thresholds for charging detection
    const float CHARGING_SLOPE_THRESHOLD =
        0.001f;  // Minimum slope to consider charging (V per sample)
    const float MIN_VOLTAGE_RANGE =
        0.005f;  // Minimum range to avoid noise false positives

    bool chargingBySlope = slope > CHARGING_SLOPE_THRESHOLD;
    bool sufficientRange = voltageRange > MIN_VOLTAGE_RANGE;
    // Only consider charging if we have both positive slope and sufficient
    // voltage range
    bool isCurrentlyCharging = chargingBySlope && sufficientRange;

    return isCurrentlyCharging;
}

void updateBatteryStatus() {
    // Single sensor read per field per tick
    voltBatteryPercent = (uint8_t)batteryService.cellPercent();

    voltBatteryVoltage = batteryService.cellVoltage();
    voltageBuffer.push(voltBatteryVoltage);

    // Calculate charge rate if we have enough data
    if (voltageBuffer.isFull()) {
        static unsigned long lastCalculation = 0;
        unsigned long now = millis();

        if (lastCalculation > 0) {
            float timeDelta = (now - lastCalculation) / 1000.0f;  // seconds
            if (timeDelta > 0) {
                // Simple rate calculation using oldest and newest values
                float voltageDelta =
                    voltageBuffer[voltageBuffer.size() - 1] - voltageBuffer[0];
                lastChargeRate = voltageDelta / timeDelta;  // V/s
            }
        }
        lastCalculation = now;
    }

    ESP_LOGV(TAG,
             "Battery Update - Percent: %d%%, Voltage: %.2fV, Charge Rate: "
             "%.2f%%/hr",
             voltBatteryPercent, voltBatteryVoltage, lastChargeRate);
}