#include "imu.h"

// Initialize the global service instance
LSM6DS3 imuInstance(0x6A);

// Private variables
static float accelX = 0, accelY = 0, accelZ = 0;
static float lastAccelX = 0, lastAccelY = 0, lastAccelZ = 0;

bool initIMUService()
{
    if (!imuInstance.begin())
    {
        Serial.println("Failed to find LSM6DS3TR-C chip");
        return false;
    }

    Serial.println("LSM6DS3TR-C Found!");
    return true;
}

void updateIMUReadings()
{
    // Read accelerometer data
    accelX = imuInstance.readFloatAccelX();
    accelY = imuInstance.readFloatAccelY();
    accelZ = imuInstance.readFloatAccelZ();

    // Store last readings
    lastAccelX = accelX;
    lastAccelY = accelY;
    lastAccelZ = accelZ;

    if (accelX != 0 || accelY != 0 || accelZ != 0)
    {
        // Serial.println("accelX: " + String(accelX) + " accelY: " + String(accelY) + " accelZ: " + String(accelZ));
    }
}