#ifndef SIZES_H
#define SIZES_H

#include <Arduino.h>

namespace Display {
    // black
    constexpr int WIDTH = 320;
    constexpr int HEIGHT = 240;

    constexpr int P1 = 6;
    constexpr int StatusbarIcons = 24;
    constexpr int StatusbarNumberOfIcons = 3;
    constexpr int StatusbarHeight = StatusbarIcons + P1;
    constexpr int StatusbarWidth =
        StatusbarIcons * StatusbarNumberOfIcons + P1 * 2;

    constexpr int PageHeight = HEIGHT - StatusbarHeight;
    constexpr int PageY = StatusbarHeight;

    constexpr int NotificationBarHeight = StatusbarHeight + StatusbarIcons + P1;

    namespace Icons {
        constexpr int Big = 120;
        constexpr int Small = 24;
        constexpr int NumIcons = 3;
    }

    namespace Padding {
        constexpr int P0 = 3;
        constexpr int P1 = 6;
        constexpr int P2 = 12;
        constexpr int P3 = 18;
        constexpr int P4 = 24;
    }
}

static int iconIdx = 0;

static int16_t getIconX(const int iconNum = iconIdx,
                        const int totalIcons = Display::Icons::NumIcons) {
    iconIdx++;
    iconIdx %= totalIcons;
    // Calculate total width of all icons including padding
    int totalWidth =
        (Display::Icons::Small + Display::Padding::P1) * totalIcons;
    // Calculate the starting X position to center all icons
    int startX = (Display::WIDTH - totalWidth) / 2;
    // Return the X position for the specific icon
    return startX + (Display::Icons::Small + Display::Padding::P1) * iconNum;
}

#endif  // SIZES_H
