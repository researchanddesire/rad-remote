#ifndef SEARCH_H
#define SEARCH_H

#include "Adafruit_ST7789.h"
#include "components/Icons.h"
#include "constants/Colors.h"
#include "constants/Sizes.h"
#include <Fonts/FreeSans9pt7b.h>
#include "services/display.h"

void drawSearchTask(void *pvParameters);

void searchForDevicesTask(void *pvParameters);

#endif