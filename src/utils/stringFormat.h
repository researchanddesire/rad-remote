#ifndef LOCKBOX_STRINGFORMAT_H
#define LOCKBOX_STRINGFORMAT_H

#include <Adafruit_GFX.h>
#include <Arduino.h>

const uint64_t SECOND = 1000;
const uint64_t MINUTE = 60 * SECOND;
const uint64_t HOUR = 60 * MINUTE;
const uint64_t DAY = 24 * HOUR;
const uint64_t MONTH = 30 * DAY; // Approximate
const uint64_t YEAR = 365 * DAY; // Approximate

static void wrapText(Adafruit_GFX &gfx, const String &text, int16_t x = 0,
                     int16_t y = 8, int16_t rightPadding = 0)
{
    int16_t width = gfx.width() - x - rightPadding;
    int16_t currentX = x;
    int16_t currentY = y;

    auto printWord = [&](const String &word)
    {
        int16_t x1, y1;
        uint16_t w, h;
        gfx.getTextBounds(word.c_str(), 0, 0, &x1, &y1, &w, &h);

        if (gfx.getCursorX() + w >= width)
        {
            gfx.println("");
            //Update the cursor to the correct X position after a line break
            gfx.setCursor(x, gfx.getCursorY());
        }

        gfx.print(word);
    };

    String word;
    for (char c : text)
    {
        word += c;
        if (c != ' ')
        {
            continue;
        }

        printWord(word);
        word = "";
    }

    // Print the last word
    printWord(word);
}

#endif // LOCKBOX_STRINGFORMAT_H