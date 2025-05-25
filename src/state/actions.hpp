#pragma once

#include <Arduino.h>
#include "events.hpp"
#include "dependencies.hpp"
#include "pages/controller.h"
#include "pages/menus.h"
#include "services/encoder.h"

namespace actions
{

    auto send_fin = [](sender &s)
    { s.send(fin{}); };

    template <typename T>
    auto send_ack = [](const T &event, sender &s)
    {
        s.send(event);
    };

    auto clearScreen = [](sender &s)
    {
        // small delay to ensure tasks are finished
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            tft.fillScreen(Colors::black);
            xSemaphoreGive(displayMutex);
        }
    };

    auto drawControl = [](sender &s)
    {
        delay(100);
        clearScreen(s);
        xTaskCreatePinnedToCore(drawControllerTask, "drawControllerTask", 10 * configMINIMAL_STACK_SIZE, NULL, 5, NULL, 1);
    };

    auto drawPatternMenu = [](sender &s)
    {
        clearScreen(s);
        xTaskCreatePinnedToCore(drawPatternMenuTask, "drawPatternMenuTask", 15 * configMINIMAL_STACK_SIZE, NULL, 5, NULL, 1);
    };

    auto drawActiveMenu = [](const MenuItem *menu, int count)
    {
        return [menu, count](sender &s)
        {
            activeMenu = menu;
            activeMenuCount = count;

            rightEncoder.setBoundaries(0, activeMenuCount - 1);
            rightEncoder.setAcceleration(0);
            rightEncoder.setEncoderValue(0);

            clearScreen(s);

            if (menuTaskHandle != nullptr)
            {
                // then the menu task is already running, so we don't need to create it again.
                return;
            }
            xTaskCreatePinnedToCore(drawMenuTask, "drawMenuTask", 5 * configMINIMAL_STACK_SIZE, NULL, 5, &menuTaskHandle, 1);
        };
    };

    auto espRestart = [](sender &s)
    {
        esp_restart();
    };

} // namespace actions