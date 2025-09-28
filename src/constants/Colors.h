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

    // OSSM default colors for depth, sensation, stroke (RGB565 format) - Stylish muted tones
    constexpr int depth = 0xE186;     // Warm Coral: RGB(224,48,104) - softer red-pink
    constexpr int sensation = 0x3C9F;  // Ocean Blue: RGB(56,146,248) - muted blue
    constexpr int stroke = 0x4E8A;     // Forest Green: RGB(76,208,80) - natural green
    constexpr int speed = 0x5013;      // Royal Purple: RGB(80,20,152) - slightly lighter than deep royal

    // very dark grey for passive/disabled coloring
    constexpr int disabled = 0x1082;

    // bg-gray-900 rgb(17 24 39);
    constexpr int bgGray900 = 0x10c5;

    // bg-gray-600 rgb(75 85 99);
    constexpr int bgGray600 = 0x4aac;

    // light gray for body text rgb(156 163 175);
    constexpr int lightGray = 0x9c71;

    // light gray for body text rgb(156 163 175);
    constexpr int textForeground = ST77XX_WHITE;
    // Slightly darker than white: use a very light gray (e.g., rgb(220,220,220)
    // ≈ 0xDEFB)
    constexpr int textForegroundSecondary = 0xDEFB;

    constexpr int textBackground = 0x4aac;

}

namespace LEDColors {
    constexpr int logoBlue = 150;
    constexpr int connected = 96;
}

#endif  // COLORS_H
