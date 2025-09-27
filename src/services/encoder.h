#ifndef ENCODER_SERVICE_H
#define ENCODER_SERVICE_H

#include <Arduino.h>

#include <AiEsp32RotaryEncoder.h>

#include "pins.h"

// Declare the global service instance
extern DRAM_ATTR AiEsp32RotaryEncoder leftEncoder;
extern DRAM_ATTR AiEsp32RotaryEncoder rightEncoder;

void initEncoderService();

// Helper functions to check and reset encoder change state
// Reading automatically resets the state (atomic check-and-clear operation)
bool hasLeftEncoderChanged();
bool hasRightEncoderChanged();

#endif  // ENCODER_SERVICE_H