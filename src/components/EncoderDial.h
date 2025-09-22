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
#include <Fonts/FreeSansBold12pt7b.h>
#include <vector>

class EncoderDial : public DisplayObject
{
private:
    bool lastButtonState = false;
    std::map<String, float *> parameters;
    const String action;
    const bool isLeft;
    const uint16_t color;
    std::vector<uint16_t> colors; // Optional per-parameter colors
    const int maxValue = 100;
    int focusedIndex = 0; // Track which arc is focused
    std::map<String, int> lastParameterValues;
    int lastFocusedIndex = -1;

    void drawArc(int arcRadius, int centerX, int centerY, int fillSteps, int steps, int circleRadius, bool isFocused, uint16_t activeColor)
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
                canvas->fillCircle(x, y, circleRadius, isFocused ? ST77XX_WHITE : activeColor);
            }
            else
            {
                canvas->fillCircle(x, y, circleRadius, 0x7BEF); // Dark gray color
            }
        }
    }

public:
    EncoderDial(const std::map<String, float *> &initialParams, const String &action, bool isLeft, int16_t x, int16_t y, int16_t width = 90, int16_t height = 90)
        : DisplayObject(x, y, width, height),
          parameters(initialParams),
          action(action),
          isLeft(isLeft),
          color(ST77XX_WHITE)
    {
    }

    // Optional: supply per-parameter colors; if empty, default color is used
    EncoderDial(const std::map<String, float *> &initialParams, const String &action, bool isLeft, const std::vector<uint16_t> &colors,
                int16_t x, int16_t y, int16_t width = 90, int16_t height = 90)
        : DisplayObject(x, y, width, height),
          parameters(initialParams),
          action(action),
          isLeft(isLeft),
          color(ST77XX_WHITE),
          colors(colors)
    {
    }

    void setFocusedIndex(int index)
    {
        if (index >= 0 && index < parameters.size() && focusedIndex != index)
        {
            focusedIndex = index;
            isDirty = true;
        }
    }

    int getFocusedIndex()
    {
        return focusedIndex;
    }

    void setParameters(const std::map<String, float *> &newParams)
    {
        if (newParams != parameters)
        {
            parameters = newParams;
            isDirty = true;
        }
    }

    // Set or update per-parameter colors at runtime
    void setColors(const std::vector<uint16_t> &newColors)
    {
        colors = newColors;
        isDirty = true;
    }

    void setParameter(int value)
    {
        if (parameters.size() == 0)
            return;

        auto it = parameters.begin();
        std::advance(it, focusedIndex);
        if (it != parameters.end() && it->second != nullptr)
        {
            int currentValue = (int)roundf(*(it->second));
            int newValue = constrain(value, 0, 100);

            // Check if we're crossing 0% or 100% boundary
            if ((currentValue != 0 && newValue == 0) || (currentValue != 100 && newValue == 100))
            {
                playVibratorPattern(VibratorPattern::SINGLE_PULSE);
                playBuzzerPattern(BuzzerPattern::SINGLE_BEEP);
            }

            *(it->second) = newValue;
            isDirty = true;
        }
    }

    int getParameter() const
    {
        ESP_LOGI("EncoderDial", "Getting parameter");
        if (parameters.empty())
        {
            return 0;
        }
        int safeIndex = focusedIndex % parameters.size();

        auto it = parameters.begin();
        std::advance(it, safeIndex);
        int value = 0;
        if (it != parameters.end() && it->second != nullptr)
        {
            value = (int)roundf(*(it->second));
        }
        ESP_LOGI("EncoderDial", "Parameter: %d", value);
        return value;
    }

    String getParameterName() const
    {
        if (parameters.size() == 0)
            return "";

        auto it = parameters.begin();
        std::advance(it, focusedIndex);
        return it != parameters.end() ? it->first : "";
    }

    // Keep the old method for backward compatibility but mark it as deprecated
    [[deprecated("Use setParameter(int value) instead")]]
    void setParameter(const String &name, int value)
    {
        auto it = parameters.find(name);
        if (it == parameters.end() || it->second == nullptr)
            return;

        int currentValue = (int)roundf(*(it->second));
        int newValue = constrain(value, 0, 100);
        if (currentValue == newValue)
            return;

        if ((currentValue != 0 && newValue == 0) || (currentValue != 100 && newValue == 100))
        {
            playVibratorPattern(VibratorPattern::SINGLE_PULSE);
            playBuzzerPattern(BuzzerPattern::SINGLE_BEEP);
        }
        *(it->second) = newValue;
        isDirty = true;
    }

    bool shouldDraw() override
    {
        if (lastFocusedIndex != focusedIndex)
        {
            return true;
        }

        if (lastParameterValues.size() != parameters.size())
        {
            return true;
        }

        for (const auto &param : parameters)
        {
            int currentValue = 0;
            if (param.second != nullptr)
            {
                currentValue = constrain((int)roundf(*(param.second)), 0, 100);
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
        canvas->fillScreen(ST77XX_BLACK);

        int steps = 100;
        int circleRadius = 2;
        int maxArcRadius = width / 2 - circleRadius;
        int arcSpacing = maxArcRadius / (parameters.size() + 1); // Space between arcs

        int centerX = width / 2;
        int centerY = height / 2;

        // Draw arcs for each parameter
        int arcIndex = 0;
        for (const auto &param : parameters)
        {
            int currentRadius = maxArcRadius - (arcIndex * arcSpacing);
            int paramValue = 0;
            if (param.second != nullptr)
            {
                paramValue = constrain((int)roundf(*(param.second)), 0, 100);
            }
            int fillSteps = (paramValue * steps) / maxValue;
            uint16_t arcColor = (arcIndex < (int)colors.size()) ? colors[arcIndex] : color;
            drawArc(currentRadius, centerX, centerY, fillSteps, steps, circleRadius, arcIndex == focusedIndex, arcColor);
            arcIndex++;
        }

        // Draw focused parameter label and value
        canvas->setTextColor(ST77XX_WHITE);

        // Find the focused parameter
        auto it = parameters.begin();
        std::advance(it, focusedIndex);
        if (it != parameters.end())
        {
            // Draw parameter name at bottom (classic font)
            String label = it->first;
            canvas->setFont(NULL);
            int16_t x1, y1;
            uint16_t w, h;
            canvas->getTextBounds(label.c_str(), 0, 0, &x1, &y1, &w, &h);
            int16_t labelCursorX = centerX - (x1 + (int16_t)(w / 2));
            int16_t labelBaselineY = height - 10; // slight margin from bottom
            canvas->setCursor(labelCursorX, labelBaselineY);
            canvas->print(label);

            int displayValue = 0;
            if (it->second != nullptr)
            {
                displayValue = constrain((int)roundf(*(it->second)), 0, 100);
            }
            String percentStr = String(displayValue);

            canvas->setFont(&FreeSansBold12pt7b);
            // Measure to center precisely with current font
            canvas->getTextBounds(percentStr.c_str(), 0, 0, &x1, &y1, &w, &h);
            int16_t valueCursorX = centerX - (x1 + (int16_t)(w / 2));
            int16_t valueBaselineY = centerY - (y1 + (int16_t)(h / 2));
            canvas->setCursor(valueCursorX, valueBaselineY);
            canvas->print(percentStr);
            // Restore default font
            canvas->setFont(NULL);
        }

        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            tft.drawRGBBitmap(x, y, canvas->getBuffer(), width, height);
            xSemaphoreGive(displayMutex);
        }

        lastParameterValues.clear();
        for (const auto &param : parameters)
        {
            int value = 0;
            if (param.second != nullptr)
            {
                value = constrain((int)roundf(*(param.second)), 0, 100);
            }
            lastParameterValues[param.first] = value;
        }
        lastFocusedIndex = focusedIndex;
    }

    int incrementFocus()
    {
        ESP_LOGI("EncoderDial", "Incrementing focus");
        if (parameters.size() > 0)
        {
            setFocusedIndex((focusedIndex + 1) % parameters.size());
            ESP_LOGI("EncoderDial", "Focused index: %d", focusedIndex);
        }
        return getParameter();
    }

    int decrementFocus()
    {
        if (parameters.size() > 0)
        {
            setFocusedIndex((focusedIndex - 1 + parameters.size()) % parameters.size());
        }
        return getParameter();
    }
};

// Initialize static member
#endif