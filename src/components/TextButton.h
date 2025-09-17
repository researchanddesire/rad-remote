#ifndef TEXTBUTTON_H
#define TEXTBUTTON_H

#include "DisplayButton.h"
#include "pins.h"
#include <Adafruit_MCP23X17.h>
#include <Adafruit_ST77xx.h>
#include "services/display.h"

class TextButton : public DisplayButton
{
private:
    static bool lastButtonState;
    const String buttonText;
    const uint8_t buttonPin;

public:
    TextButton(const String &text, uint8_t pin, int16_t x, int16_t y, int16_t width = 60, int16_t height = 25)
        : DisplayButton(x, y, width, height), buttonText(text), buttonPin(pin)
    {
    }

    bool shouldDraw() override
    {
        bool currentState = digitalRead(buttonPin) == LOW;
        bool shouldRedraw = currentState != lastButtonState;
        lastButtonState = digitalRead(buttonPin) == LOW;
        return true;
    }

    void draw() override
    {
        canvas->fillScreen(ST77XX_BLACK);

        // Draw button background
        if (!lastButtonState)
        {
            canvas->drawRoundRect(0, 0, width, height, 5, 0x7BEF); // Dark grey color
            canvas->setTextColor(0x7BEF);                          // Dark grey color
        }
        else
        {
            canvas->fillRoundRect(0, 0, width, height, 5, ST77XX_WHITE);
            canvas->setTextColor(ST77XX_BLACK);
        }

        // Draw text
        canvas->setTextSize(1);

        // Get text bounds to center it
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        canvas->getTextBounds(buttonText.c_str(), 0, 0, &x1, &y1, &textWidth, &textHeight);

        // Calculate center position
        int16_t textX = (width - textWidth) / 2;
        int16_t textY = (height - textHeight) / 2 - y1; // Subtract y1 to account for baseline and offset

        canvas->setCursor(textX, textY);
        canvas->print(buttonText);

        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            tft.drawRGBBitmap(x, y, canvas->getBuffer(), width, height);
            xSemaphoreGive(displayMutex);
        }
    }
};

// Initialize static member
bool TextButton::lastButtonState = false;
#endif