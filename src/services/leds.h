#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"

extern Adafruit_NeoPixel strip;

void initLeds();

extern uint8_t brightness;

#endif