#ifndef LEFT_ENCODER_MONITOR_SERVICE_H
#define LEFT_ENCODER_MONITOR_SERVICE_H

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Start global left encoder monitoring for device states
void startLeftEncoderMonitoring();

// Stop left encoder monitoring (cleanup)
void stopLeftEncoderMonitoring();

#endif  // LEFT_ENCODER_MONITOR_SERVICE_H