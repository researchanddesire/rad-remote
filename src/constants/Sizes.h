#ifndef SIZES_H
#define SIZES_H

namespace Display
{
    // black
    constexpr int WIDTH = 320;
    constexpr int HEIGHT = 240;

    constexpr int P1 = 6;
    constexpr int StatusbarIcons = 24;
    constexpr int StatusbarHeight = StatusbarIcons + P1;
    constexpr int NotificationBarHeight = StatusbarHeight + StatusbarIcons + P1;

    namespace Icons
    {
        constexpr int Big = 120;
        constexpr int Small = 24;
    }

    namespace Padding
    {
        constexpr int P0 = 3;
        constexpr int P1 = 6;
        constexpr int P2 = 12;
        constexpr int P3 = 18;
        constexpr int P4 = 24;
    }
}

static int16_t getIconX(const int iconNum)
{
    return Display::WIDTH - (Display::Icons::Small + Display::Padding::P1) * (iconNum + 1) + Display::Padding::P1;
}

#endif // SIZES_H
