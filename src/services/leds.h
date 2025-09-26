#ifndef LEDS_SERVICE_H
#define LEDS_SERVICE_H

#include <FastLED.h>
#include <pins.h>

#define BRIGHTNESS 255
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

extern CRGB leds[pins::NUM_LEDS];
extern float ledPaceSpeedRpm;

void initFastLEDs();

void setLed(uint8_t color_value, uint8_t brightness = 255,
            uint16_t duration_ms = 250);
void setLedOff();

#endif  // LEDS_SERVICE_H
