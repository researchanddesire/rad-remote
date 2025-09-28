﻿#ifndef TextButton_h
#define TextButton_h

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans9pt7b.h>  // Match genericPages button font
#include "constants/Colors.h"
#include "DisplayObject.h"
#include "../services/display.h"

extern Adafruit_ST7789 tft;
extern SemaphoreHandle_t displayMutex;

class TextButton : public DisplayObject
{
private:
    bool lastButtonState = false;
    String buttonText;
    const uint8_t buttonPin;
    
    // Dynamic styling properties
    uint16_t textColor = Colors::black;
    uint16_t backgroundColor = Colors::textBackground;
    uint16_t pressedTextColor = Colors::black;
    uint16_t pressedBackgroundColor = Colors::white;
    
    // Track changes for redraw
    String lastText;
    uint16_t lastTextColor;
    uint16_t lastBackgroundColor;

public:
    TextButton(const String &text, uint8_t pin, int16_t x, int16_t y, int16_t width = 70, int16_t height = 35)
        : DisplayObject(x, y, width, height), buttonText(text), buttonPin(pin)
    {
        // Initialize tracking variables
        lastText = text;
        lastTextColor = textColor;
        lastBackgroundColor = backgroundColor;
    }

    // Methods to dynamically change button appearance
    void setText(const String &text) {
        buttonText = text;
    }

    void setColors(uint16_t backgroundColor = Colors::textBackground, uint16_t textColor = Colors::white) {
        this->textColor = textColor;
        this->backgroundColor = backgroundColor;
    }

    bool shouldDraw() override
    {
        bool currentState = digitalRead(buttonPin) == LOW;
        bool stateChanged = currentState != lastButtonState;
        bool textChanged = buttonText != lastText;
        bool colorsChanged = (textColor != lastTextColor ||
                             backgroundColor != lastBackgroundColor);
        
        return stateChanged || textChanged || colorsChanged || isFirstDraw;
    }

    void draw() override
    {
        bool currentState = digitalRead(buttonPin) == LOW;
        
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Clear the button area
            tft.fillRect(x, y, width, height, ST77XX_BLACK);
            
            tft.setFont(&FreeSans9pt7b);
            
            if (!currentState) {
                // Use filled rectangle for improved visuals 
                //(border-only near physical screen border can cause visual artifacts)
                tft.fillRoundRect(x, y, width, height, 5, backgroundColor);
                tft.setTextColor(textColor);
            } else {                
                tft.fillRoundRect(x, y, width, height, 5, pressedBackgroundColor);
                tft.setTextColor(pressedTextColor);
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
        
        // Update tracking variables
        lastButtonState = currentState;
        lastText = buttonText;
        lastTextColor = textColor;
        lastBackgroundColor = backgroundColor;
        isFirstDraw = false;
    }
};

#endif