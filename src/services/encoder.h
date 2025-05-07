#ifndef ENCODER_SERVICE_H
#define ENCODER_SERVICE_H

#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include "pins.h"

// Declare the global service instance
extern AiEsp32RotaryEncoder leftEncoder;
extern AiEsp32RotaryEncoder rightEncoder;

// Function declarations
void initEncoderService();
void IRAM_ATTR readLeftEncoder();
void IRAM_ATTR readRightEncoder();

#endif // ENCODER_SERVICE_H