#ifndef TextButton_h
#define TextButton_h

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans9pt7b.h>  // Match genericPages button font
#include "DisplayObject.h"
#include "../services/display.h"

extern Adafruit_ST7789 tft;
extern SemaphoreHandle_t displayMutex;

class TextButton : public DisplayObject
{
private:
    bool lastButtonState = false;
    const String buttonText;
    const uint8_t buttonPin;

public:
    TextButton(const String &text, uint8_t pin, int16_t x, int16_t y, int16_t width = 70, int16_t height = 30)
        : DisplayObject(x, y, width, height), buttonText(text), buttonPin(pin)
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
        
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Clear the button area
            tft.fillRect(x, y, width, height, ST77XX_BLACK);
            
            tft.setFont(&FreeSans9pt7b);
            
            if (!currentState) {
                tft.drawRoundRect(x, y, width, height, 5, 0x7BEF);
                tft.setTextColor(0x7BEF);
            } else {
                tft.fillRoundRect(x, y, width, height, 5, ST77XX_WHITE);
                tft.setTextColor(ST77XX_BLACK);
            }
            
            // Calculate text position for proper centering (matching genericPages.cpp style)
            int16_t x1, y1;
            uint16_t textWidth, textHeight;
            tft.getTextBounds(buttonText.c_str(), 0, 0, &x1, &y1, &textWidth, &textHeight);
            
            // Horizontal centering
            int16_t textX = x + (width - textWidth) / 2;
            
            // Vertical centering - using same formula as Device Stopped screen buttons
            int16_t textY = y + (height + textHeight) / 2;
            
            tft.setCursor(textX, textY);
            tft.print(buttonText);
            
            xSemaphoreGive(displayMutex);
        }
        
        lastButtonState = currentState;
    }
};

#endif