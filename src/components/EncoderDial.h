#pragma once

#include "DisplayButton.h"
#include "pins.h"
#include <Adafruit_MCP23X17.h>
#include <Adafruit_ST77xx.h>
#include "services/display.h"
#include "services/mcp.h"
#include <map>
#include "utils/vibrator.h"
#include "utils/buzzer.h"
#include "esp_log.h"

class EncoderDial : public DisplayButton
{
private:
    static bool lastButtonState;
    std::map<String, int> parameters;
    const String action;
    const bool isLeft;
    const uint16_t color;
    const int maxValue = 100;
    int focusedIndex = 0; // Track which arc is focused

    void drawArc(int arcRadius, int centerX, int centerY, int fillSteps, int steps, int circleRadius, bool isFocused)
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
                canvas->fillCircle(x, y, circleRadius, isFocused ? ST77XX_WHITE : 0xAD55);
            }
            else
            {
                canvas->fillCircle(x, y, circleRadius, 0x7BEF); // Dark gray color
            }
        }
    }

public:
    EncoderDial(const std::map<String, int> &initialParams, const String &action, bool isLeft, int16_t x, int16_t y, int16_t width = 90, int16_t height = 90)
        : DisplayButton(x, y, width, height),
          parameters(initialParams),
          action(action),
          isLeft(isLeft),
          color(ST77XX_WHITE)
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

    void setParameters(const std::map<String, int> &newParams)
    {
        if (newParams != parameters)
        {
            parameters = newParams;
            isDirty = true;
        }
    }

    void setParameter(int value)
    {
        if (parameters.size() == 0)
            return;

        auto it = parameters.begin();
        std::advance(it, focusedIndex);
        if (it != parameters.end() && it->second != value)
        {
            // Check if we're crossing 0% or 100% boundary
            if ((it->second != 0 && value == 0) || (it->second != 100 && value == 100))
            {
                playVibratorPattern(VibratorPattern::SINGLE_PULSE);
                playBuzzerPattern(BuzzerPattern::SINGLE_BEEP);
            }

            it->second = value;
            isDirty = true;
        }
    }

    int getParameter() const
    {
        ESP_LOGI("EncoderDial", "Getting parameter");
        int safeIndex = focusedIndex % parameters.size();

        auto it = parameters.begin();
        std::advance(it, safeIndex);
        ESP_LOGI("EncoderDial", "Parameter: %d", it->second);
        return it->second;
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
        if (parameters[name] != value)
        {
            parameters[name] = value;
            isDirty = true;
        }
    }

    bool shouldDraw() override
    {
        return true;
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
            int fillSteps = (param.second * steps) / maxValue;
            drawArc(currentRadius, centerX, centerY, fillSteps, steps, circleRadius, arcIndex == focusedIndex);
            arcIndex++;
        }

        // Draw focused parameter label at bottom
        canvas->setTextSize(1);
        canvas->setTextColor(ST77XX_WHITE);

        // Find the focused parameter
        auto it = parameters.begin();
        std::advance(it, focusedIndex);
        if (it != parameters.end())
        {
            // Draw parameter name at bottom
            String label = it->first;
            int16_t textWidth = label.length() * 6; // 6 pixels per character at text size 1
            canvas->setCursor(centerX - textWidth / 2, height - 10);
            canvas->print(label);

            // Draw large percentage in center
            String percentStr = String(it->second);
            textWidth = percentStr.length() * 6; // Approximate width for 9pt font
            canvas->setCursor(centerX - textWidth / 2, centerY - 10);
            canvas->print(percentStr);
        }

        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            tft.drawRGBBitmap(x, y, canvas->getBuffer(), width, height);
            xSemaphoreGive(displayMutex);
        }
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
bool EncoderDial::lastButtonState = false;
