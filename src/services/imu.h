#ifndef IMU_SERVICE_H
#define IMU_SERVICE_H

#include <Arduino.h>
#include "SparkFunLSM6DS3.h"

// Declare the global service instance
extern LSM6DS3 imuInstance;

// Function declarations
bool initIMUService();
void updateIMUReadings();

#endif // IMU_SERVICE_H