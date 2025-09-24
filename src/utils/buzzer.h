#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "pins.h"

// Pattern types
enum class BuzzerPattern
{
    SINGLE_BEEP,
    DOUBLE_BEEP,
    TRIPLE_BEEP,
    ERROR_BEEP
};

// Initialize the buzzer system
void initBuzzer();

// Play a specific pattern
void playBuzzerPattern(BuzzerPattern pattern);

// Stop any currently playing pattern
void stopBuzzer();