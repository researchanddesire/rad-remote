#ifndef COLORS_H
#define COLORS_H

#include <Adafruit_ST77xx.h>

namespace Colors {
    // black
    constexpr int black = 0x0000;
    constexpr int white = ST77XX_WHITE;
    constexpr int red = ST77XX_RED;
    constexpr int dimRed = 0x8000;
    constexpr int green = ST77XX_GREEN;
    constexpr int yellow = ST77XX_YELLOW;

    // bg-gray-900 rgb(17 24 39);
    constexpr int bgGray900 = 0x10c5;

    // bg-gray-600 rgb(75 85 99);
    constexpr int bgGray600 = 0x4aac;

    // light gray for body text rgb(156 163 175);
    constexpr int lightGray = 0x9c71;

}

namespace LEDColors {
    constexpr int logoBlue = 150;
    constexpr int connected = 96;
}

#endif  // COLORS_H
