#include "display.h"

// Screen setup
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, pins::TFT_CS, pins::TFT_A0, pins::TFT_RST);
