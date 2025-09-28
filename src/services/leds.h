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

// Individual LED control functions
void setIndividualLed(uint8_t ledIndex, uint16_t rgb565Color, uint8_t brightness = 255);
void setLeftEncoderLed(uint16_t rgb565Color, uint8_t brightness = 255);
void setRightEncoderLed(uint16_t rgb565Color, uint8_t brightness = 255);
void setMiddleLed(uint16_t rgb565Color, uint8_t brightness = 255);
void releaseIndividualLed(uint8_t ledIndex);  // Release LED back to global control

#endif  // LEDS_SERVICE_H
