#ifndef MENUS_H
#define MENUS_H

#include "structs/Menus.h"
#include "Adafruit_ST7789.h"
#include "components/Icons.h"
#include "constants/Colors.h"
#include "constants/Sizes.h"
#include <Fonts/FreeSans9pt7b.h>
#include "services/display.h"

extern const MenuItem *activeMenu;
extern int activeMenuCount;
extern int currentOption;

void drawMenu(Adafruit_ST7789 *display);
void drawMenuTask(void *pvParameters);

#endif