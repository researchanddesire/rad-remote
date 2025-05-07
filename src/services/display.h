#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_ST7789.h>
#include "pins.h"

// Global TFT instance
extern Adafruit_ST7789 tft;

// Initialize the display
bool initDisplay();

#endif
