#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_ST7789.h>
#include "pins.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Global TFT instance
extern Adafruit_ST7789 tft;

// Display semaphore for thread-safe access
extern SemaphoreHandle_t displayMutex;

// Initialize the display
bool initDisplay();

#endif
