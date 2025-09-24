#ifndef ICONBUTTON_H
#define ICONBUTTON_H

#include "DisplayObject.h"
#include "pins.h"
#include <Adafruit_MCP23X17.h>
#include <Adafruit_ST77xx.h>
#include "services/display.h"
#include "Icons.h"

class IconButton : public DisplayObject
{
private:
    bool lastButtonState = false;
    const unsigned char *iconBitmap; // 1-bit PROGMEM bitmap, 24x24
    const uint8_t buttonPin;

    static constexpr int16_t ICON_SIZE = 24;

public:
    IconButton(const unsigned char *icon, uint8_t pin, int16_t x, int16_t y, int16_t width = 32, int16_t height = 32)
        : DisplayObject(x, y, width, height), iconBitmap(icon), buttonPin(pin)
    {
    }

    bool shouldDraw() override
    {
        bool currentState = digitalRead(buttonPin) == LOW;
        return currentState != lastButtonState;
    }

    void draw() override
    {
        bool currentState = digitalRead(buttonPin) == LOW;
        canvas->fillScreen(ST77XX_BLACK);

        uint16_t fgColor;

        // Draw button background
        if (!currentState)
        {
            canvas->drawRoundRect(0, 0, width, height, 5, 0x7BEF); // Dark grey color
            fgColor = 0x7BEF;
        }
        else
        {
            canvas->fillRoundRect(0, 0, width, height, 5, ST77XX_WHITE);
            fgColor = ST77XX_BLACK;
        }

        // Center and draw the 24x24 icon
        int16_t iconX = (width - ICON_SIZE) / 2;
        int16_t iconY = (height - ICON_SIZE) / 2;
        canvas->drawBitmap(iconX, iconY, iconBitmap, ICON_SIZE, ICON_SIZE, fgColor);

        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            tft.drawRGBBitmap(x, y, canvas->getBuffer(), width, height);
            xSemaphoreGive(displayMutex);
        }

        lastButtonState = currentState;
    }
};

#endif
