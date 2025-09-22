#pragma once

#ifndef LINEARRAILGRAPH_H
#define LINEARRAILGRAPH_H

#include "DisplayObject.h"
#include "pins.h"
#include <Adafruit_MCP23X17.h>
#include <Adafruit_ST77xx.h>
#include "services/display.h"
#include <map>
#include "utils/vibrator.h"
#include "utils/buzzer.h"
#include "esp_log.h"
#include <devices/researchAndDesire/ossm/ossm_device.hpp>
#include <constants.h>

class LinearRailGraph : public DisplayObject
{
private:
    OSSM *ossm;

public:
    LinearRailGraph(OSSM *ossm, int16_t x = -1, int16_t y = -1, int16_t width = 90, int16_t height = 30)
        : DisplayObject(
              (x == -1) ? (DISPLAY_WIDTH / 2 - width / 2) : x,
              (y == -1) ? (DISPLAY_HEIGHT / 2 - height / 2) : y,
              width, height),
          ossm(ossm)
    {
        // No further adjustment needed; handled in initializer list
    }

    bool shouldDraw() override
    {
        return true;
    }

    void draw() override
    {
        canvas->fillScreen(ST77XX_BLACK);

        canvas->drawRoundRect(0, 0, width, height, 4, COLOR_WHITE);

        // then draw a rounded rect for the current "stroke" and "depth"
        int strokeWidth = constrain(int(ossm->getStroke() * float(width) / 100.0f), 2, width);
        int depth = int((width - strokeWidth) * ossm->getDepth() / 100.0f);

        canvas->fillRoundRect(depth, 0, strokeWidth, height, 4, COLOR_WHITE);

        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            tft.drawRGBBitmap(x, y, canvas->getBuffer(), width, height);
            xSemaphoreGive(displayMutex);
        }
    }
};

// Initialize static member
#endif