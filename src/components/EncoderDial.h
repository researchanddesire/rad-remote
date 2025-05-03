#pragma once

#include "DisplayButton.h"
#include "pins.h"
#include <Adafruit_MCP23X17.h>
#include <Adafruit_ST77xx.h>
#include "services/display.h"
#include "services/mcp.h"

class EncoderDial : public DisplayButton
{
private:
    static bool lastButtonState;
    mutable String label;
    const String action;
    const bool isLeft;
    int value;
    const int maxValue;
    const uint16_t color;

    void fillArc(int centerX, int centerY, int innerRadius, int outerRadius, int startAngle, int endAngle, uint16_t color)
    {
        // Convert angles to radians
        float startRad = startAngle * PI / 180.0;
        float endRad = endAngle * PI / 180.0;

        // Calculate optimal number of steps based on arc size (angle difference)
        // More steps for larger arcs to ensure smooth rendering
        int arcDegrees = abs(endAngle - startAngle);
        int steps = max(arcDegrees, 12); // At least 1 step per degree, minimum 12 steps
        steps = min(steps, 256);         // Cap at 256 steps for performance

        // Pre-calculate angle step
        float angleStep = (endRad - startRad) / steps;

        // Draw triangles for each step
        for (int i = 0; i < steps; i++)
        {
            float angle1 = startRad + angleStep * i;
            float angle2 = startRad + angleStep * (i + 1);

            // Calculate points for the triangle
            int x1 = centerX + innerRadius * cos(angle1);
            int y1 = centerY + innerRadius * sin(angle1);
            int x2 = centerX + outerRadius * cos(angle1);
            int y2 = centerY + outerRadius * sin(angle1);
            int x3 = centerX + outerRadius * cos(angle2);
            int y3 = centerY + outerRadius * sin(angle2);

            // Draw filled triangle
            canvas->fillTriangle(x1, y1, x2, y2, x3, y3, color);

            // Draw second triangle to complete the segment
            x3 = centerX + innerRadius * cos(angle2);
            y3 = centerY + innerRadius * sin(angle2);
            canvas->fillTriangle(x1, y1, x2, y2, x3, y3, color);
        }
    }

public:
    EncoderDial(const String &label, const String &action, bool isLeft, int16_t x, int16_t y, int16_t width = 60, int16_t height = 60)
        : DisplayButton(x, y, width, height),
          label(label),
          action(action),
          isLeft(isLeft),
          value(0),
          maxValue(100),
          color(ST77XX_WHITE)
    {
    }

    void setValue(int newValue)
    {
        if (newValue != value)
        {
            value = newValue;
            isDirty = true;
        }
    }

    void setTextAndValue(const String &newLabel, int newValue)
    {
        bool needsUpdate = false;
        if (newLabel != label)
        {
            label = newLabel;
            needsUpdate = true;
        }
        if (newValue != value)
        {
            value = newValue;
            needsUpdate = true;
        }
        if (needsUpdate)
        {
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

        // Draw background circles
        int centerX = width / 2;
        int centerY = height / 2;
        int outerRadius = width / 2;
        int innerRadius = width / 2 - 4;

        // Draw the base circles
        canvas->drawCircle(centerX, centerY, outerRadius, ST77XX_WHITE); // Dark grey color
        canvas->drawCircle(centerX, centerY, innerRadius, ST77XX_WHITE); // Dark grey color

        // Fill the arc based on value percentage, starting from top (90 degrees)
        int fillAngle = (value * 360) / maxValue;
        fillArc(centerX, centerY, innerRadius, outerRadius, 90, 90 + fillAngle, ST77XX_WHITE);

        // Draw label
        canvas->setTextSize(1);
        canvas->setTextColor(ST77XX_WHITE);
        int16_t textWidth = label.length() * 6;
        canvas->setCursor(centerX - textWidth / 2, centerY - 6);
        canvas->print(label);

        // Draw percentage
        String percentStr = String(value) + "%";
        textWidth = percentStr.length() * 6;
        canvas->setCursor(centerX - textWidth / 2, centerY + 6);
        canvas->print(percentStr);

        // // Draw action indicator
        // int circleX = isLeft ? centerX + width / 2 : centerX - width / 2;
        // int circleY = centerY + width / 2;
        // canvas->drawCircle(circleX, circleY, 5, 0x7BEF);

        // // Draw action text
        // textWidth = action.length() * 6;
        // canvas->setCursor(circleX - textWidth / 2, circleY + 8);
        // canvas->print(action);

        // Draw the canvas to the screen
        tft.drawRGBBitmap(x, y, canvas->getBuffer(), width, height);
    }
};

// Initialize static member
bool EncoderDial::lastButtonState = false;
