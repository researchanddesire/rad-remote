#ifndef ENCODER_SERVICE_H
#define ENCODER_SERVICE_H

#include <Arduino.h>

#include <AiEsp32RotaryEncoder.h>

#include "pins.h"

// Declare the global service instance
extern DRAM_ATTR AiEsp32RotaryEncoder leftEncoder;
extern DRAM_ATTR AiEsp32RotaryEncoder rightEncoder;

void initEncoderService();

#endif  // ENCODER_SERVICE_H