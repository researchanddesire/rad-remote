#include "leds.h"

Adafruit_NeoPixel strip(pins::NUM_LEDS, pins::LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t brightness = 200;

void initLeds()
{
    // Initialize LED strip
    strip.begin();
    strip.setBrightness(brightness);
    strip.show(); // Initialize all pixels to 'off'

    // Set all pixels to green
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(0, 255, 0)); // RGB: Green
    }
    strip.show();
}
