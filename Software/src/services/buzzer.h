#pragma once

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "pins.h"

// Pattern types
enum class BuzzerPattern {
    NONE,
    MARIO_COIN,
    SINGLE_BEEP,
    DOUBLE_BEEP,
    TRIPLE_BEEP,
    ERROR_BEEP,
    BOOT,
    SHUTDOWN,
    DEVICE_CONNECTED,
    DEVICE_DISCONNECTED,
    PAUSED,
    PLAY
};

// Initialize the buzzer system
void initBuzzer();

// Play a specific pattern
void playBuzzerPattern(BuzzerPattern pattern);

// Stop any currently playing pattern
void stopBuzzer();

// Play the Mario coin jingle after initialization
void playMarioCoin();