#pragma once

#ifndef ENCODERDIAL_H
#define ENCODERDIAL_H

#include "DisplayObject.h"
#include "pins.h"
#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_ST77xx.h>
#include "services/display.h"
#include <map>
#include "utils/vibrator.h"
#include "utils/buzzer.h"
#include "esp_log.h"
// Adafruit GFX fonts
#include <Fonts/FreeSansBold9pt7b.h>  // Reduced from 12pt to 9pt for better fit
#include <vector>
#include <AiEsp32RotaryEncoder.h>

class EncoderDial : public DisplayObject
{
private:
    bool lastButtonState = false;
    std::map<String, float *> parameters;
    const uint16_t color;
    std::vector<uint16_t> colors; // Optional per-parameter colors
    int minValue = 0;
    int maxValue = 100;
    int *focusedIndex = nullptr;
    std::map<String, int> lastParameterValues;
    int lastFocusedIndex = -1;
    bool isFirstDraw = true; // Track if this is the first draw to avoid unnecessary clearing

    void drawArcDirect(int arcRadius, int centerX, int centerY, int fillSteps, int steps, int circleRadius, bool isFocused, uint16_t activeColor)
    {
        int startAngle = 0;
        int endAngle = 270;

        for (int i = steps - 1; i >= 0; i--)
        {
            float angle = (startAngle + (i * (endAngle - startAngle) / steps)) * PI / 180.0;
            int x = centerX + arcRadius * cos(angle + 3 * PI / 4);
            int y = centerY + arcRadius * sin(angle + 3 * PI / 4);
            if (i < fillSteps || i == 0)
            {
                tft.fillCircle(x, y, circleRadius, isFocused ? ST77XX_WHITE : activeColor);
            }
            else
            {
                tft.fillCircle(x, y, circleRadius, 0x7BEF); // Dark gray color
            }
        }
    }

    AiEsp32RotaryEncoder &encoder;

public:
    struct Props
    {
        AiEsp32RotaryEncoder *encoder = nullptr;
        std::map<String, float *> parameters;
        int *focusedIndex = nullptr;
        int16_t x = -1;
        int16_t y = -1;
        int16_t width = 90;
        int16_t height = 90;
        int minValue = 0;
        int maxValue = 100;
    };

    explicit EncoderDial(const Props &props)
        : DisplayObject(props.x, props.y, props.width, props.height),
          parameters(props.parameters),
          color(ST77XX_WHITE),
          encoder(*props.encoder)
    {
        focusedIndex = props.focusedIndex;

        minValue = props.minValue;
        maxValue = props.maxValue;

        if (x == -1)
        {
            x = Display::WIDTH / 2 - width / 2;
        }

        if (y == -1)
        {
            y = Display::PageY + Display::PageHeight / 2 - height / 2;
        }
    }

    void setParameters(const std::map<String, float *> &newParams)
    {
        if (newParams != parameters)
        {
            parameters = newParams;
            isDirty = true;
        }
    }

    bool shouldDraw() override
    {

        // use focused index to get the parameter value
        int currentValue = encoder.readEncoder();
        if (parameters.size() > 0)
        {
            auto it = parameters.begin();
            std::advance(it, *focusedIndex);
            if (it != parameters.end() && it->second != nullptr)
            {
                *(it->second) = constrain(currentValue, minValue, maxValue);
            }
        }

        if (lastFocusedIndex != *focusedIndex)
        {
            return true;
        }

        if (lastParameterValues.size() != parameters.size())
        {
            return true;
        }

        for (const auto &param : parameters)
        {
            auto currentValue = 0;
            if (param.second != nullptr)
            {
                currentValue = *(param.second);
            }
            auto it = lastParameterValues.find(param.first);
            if (it == lastParameterValues.end() || it->second != currentValue)
            {
                return true;
            }
        }

        return false;
    }

    void draw() override
    {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            // Only clear entire area on first draw or focus change
            if (isFirstDraw || lastFocusedIndex != *focusedIndex)
            {
                tft.fillRect(x, y, width, height, ST77XX_BLACK);
                isFirstDraw = false;
            }
            else
            {
                // For parameter value updates, we'll redraw over existing arcs
                // This reduces flicker significantly
            }

            int steps = 100;
            int circleRadius = 2;
            int maxArcRadius = width / 2 - circleRadius;
            int arcSpacing = maxArcRadius / (parameters.size() + 1); // Space between arcs

            int centerX = x + width / 2; 
            int centerY = y + height / 2; 

            // Draw arcs for each parameter
            int arcIndex = 0;
            for (const auto &param : parameters)
            {
                int currentRadius = maxArcRadius - (arcIndex * arcSpacing);
                int paramValue = 0;
                if (param.second != nullptr)
                {
                    paramValue = constrain((int)roundf(*(param.second)), minValue, maxValue);
                }
                int range = maxValue - minValue;
                int fillSteps = range > 0 ? (int)lround(((double)(paramValue - minValue) * steps) / (double)range) : steps;
                uint16_t arcColor = (arcIndex < (int)colors.size()) ? colors[arcIndex] : color;
                
                // Only redraw arc if value changed or if it's focused parameter or first draw
                auto lastValueIt = lastParameterValues.find(param.first);
                bool valueChanged = (lastValueIt == lastParameterValues.end() || lastValueIt->second != paramValue);
                bool isFocused = (arcIndex == *focusedIndex);
                
                if (valueChanged || isFocused || isFirstDraw || lastFocusedIndex != *focusedIndex)
                {
                    drawArcDirect(currentRadius, centerX, centerY, fillSteps, steps, circleRadius, isFocused, arcColor);
                }
                
                arcIndex++;
            }

            // Draw focused parameter label and value with maximum width clearing
            tft.setTextColor(ST77XX_WHITE);

            // Find the focused parameter
            auto it = parameters.begin();
            std::advance(it, *focusedIndex);
            if (it != parameters.end())
            {
                // Get current and previous text bounds for clearing
                String label = it->first;
                int displayValue = 0;
                if (it->second != nullptr)
                {
                    displayValue = constrain((int)roundf(*(it->second)), minValue, maxValue);
                }
                String percentStr = String(displayValue);

                // Measure label text bounds with classic font
                tft.setFont(NULL);
                int16_t x1, y1;
                uint16_t w, h;
                tft.getTextBounds(label.c_str(), 0, 0, &x1, &y1, &w, &h);
                int16_t labelCursorX = centerX - (x1 + (int16_t)(w / 2));
                int16_t labelBaselineY = y + height - 10; // slight margin from bottom
                
                // Clear only the exact label area (with small padding)
                tft.fillRect(labelCursorX + x1 - 2, labelBaselineY + y1 - 1, w + 4, h + 2, ST77XX_BLACK);
                
                // Draw parameter name
                tft.setCursor(labelCursorX, labelBaselineY);
                tft.print(label);

                // Calculate maximum possible text width to prevent artifacts
                tft.setFont(&FreeSansBold9pt7b);  // Use smaller 9pt font
                
                // Get bounds for maximum possible value to determine clearing area
                String maxValueStr = String(maxValue);
                int16_t maxX1, maxY1;
                uint16_t maxW, maxH;
                tft.getTextBounds(maxValueStr.c_str(), 0, 0, &maxX1, &maxY1, &maxW, &maxH);
                
                // Center the clearing area based on maximum width
                int16_t maxValueCursorX = centerX - (maxX1 + (int16_t)(maxW / 2));
                int16_t valueBaselineY = centerY - (maxY1 + (int16_t)(maxH / 2));
                
                // Clear area large enough for maximum possible text width (with padding)
                tft.fillRect(maxValueCursorX + maxX1 - 3, valueBaselineY + maxY1 - 2, maxW + 6, maxH + 4, ST77XX_BLACK);
                
                // Now get actual text positioning for current value
                tft.getTextBounds(percentStr.c_str(), 0, 0, &x1, &y1, &w, &h);
                int16_t valueCursorX = centerX - (x1 + (int16_t)(w / 2));
                
                // Draw value at proper centered position
                tft.setCursor(valueCursorX, valueBaselineY);
                tft.print(percentStr);
                
                // Restore default font
                tft.setFont(NULL);
            }

            xSemaphoreGive(displayMutex);
        }

        lastParameterValues.clear();
        for (const auto &param : parameters)
        {
            int value = 0;
            if (param.second != nullptr)
            {
                value = constrain((int)roundf(*(param.second)), minValue, maxValue);
            }
            lastParameterValues[param.first] = value;
        }
        lastFocusedIndex = *focusedIndex;
    }
};

// Initialize static member
#endif