#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "pins.h"

// Pattern types
enum class VibratorPattern
{
    SINGLE_PULSE, // Single vibration pulse
    DOUBLE_PULSE, // Two vibration pulses
    TRIPLE_PULSE, // Three vibration pulses
    ERROR_PULSE   // Long error pulse
};

// Initialize the vibrator system
void initVibrator();

// Play a specific pattern
void playVibratorPattern(VibratorPattern pattern);

// Stop any currently playing pattern
void stopVibrator();