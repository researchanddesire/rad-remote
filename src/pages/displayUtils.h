#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include "Adafruit_ST7789.h"
#include "constants/Colors.h"
#include "constants/Sizes.h"

// Draw the vertical scrollbar for menus
void drawScrollBar(int currentOption, int numOptions);
void clearPage(bool clearStatusbar = false);

#endif