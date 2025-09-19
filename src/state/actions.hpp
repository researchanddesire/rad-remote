#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "events.hpp"
#include "pages/controller.h"
#include "pages/menus.h"
#include "services/encoder.h"
#include "pages/search.h"
#include "components/TextPages.h"
#include <devices/device.h>
#include <pages/displayUtils.h>

namespace actions
{

    auto clearScreen = []()
    {
        // small delay to ensure tasks are finished
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            tft.fillScreen(Colors::black);
            xSemaphoreGive(displayMutex);
        }
    };

    auto drawPage = [](const TextPage &page)
    {
        // TODO: Include icon.
        return [page]()
        {
            clearScreen();
            TextPage *params = new TextPage{page};
            xTaskCreatePinnedToCore([](void *pvParameters)
                                    {
                                    TextPage *params = static_cast<TextPage *>(pvParameters);
                                    String localTitle = params ? params->title : String("");
                                    String localDescription = params ? params->description : String("");
                                    delete params;

                                    int16_t width = Display::WIDTH;
                                    int16_t height = Display::PageHeight;
                                    GFXcanvas16 *canvas = new GFXcanvas16(width, height);
                                    if (canvas == nullptr)
                                    {
                                        vTaskDelete(NULL);
                                        return;
                                    }

                                    canvas->fillScreen(Colors::black);
                                    canvas->setTextSize(2);
                                    canvas->setTextColor(Colors::white);
                                    canvas->setCursor(0, 0);
                                    canvas->println(localTitle);
                                    canvas->println(localDescription);

                                    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE)
                                    {
                                        tft.drawRGBBitmap(0, Display::PageY, canvas->getBuffer(), width, height);
                                        xSemaphoreGive(displayMutex);
                                    }

                                    delete canvas;
                                    canvas = nullptr;
                                    vTaskDelete(NULL); },
                                    "drawPageTask", 5 * configMINIMAL_STACK_SIZE, params, 5, NULL, 1);
        };
    };

    auto drawControl = []()
    {
        // delay(100);
        // clearScreen();

        // rightEncoder.setBoundaries(0, 100, false);
        // rightEncoder.setAcceleration(0);
        // rightEncoder.setEncoderValue(100);

        // leftEncoder.setBoundaries(0, 100, false);
        // leftEncoder.setAcceleration(0);
        // leftEncoder.setEncoderValue(100);

        // xTaskCreatePinnedToCore(drawControllerTask, "drawControllerTask", 10 * configMINIMAL_STACK_SIZE, NULL, 5, NULL, 1);

        // if (device == nullptr)
        // {
        //     ESP_LOGW(TAG, "Device is nullptr");
        //     return;
        // }

        vTaskDelay(pdMS_TO_TICKS(10));
        clearPage();
        vTaskDelay(pdMS_TO_TICKS(10));
        device->drawControls();
    };

    auto drawStop = []()
    {
        delay(100);
        clearScreen();
        xTaskCreatePinnedToCore(drawStopTask, "drawStopTask", 10 * configMINIMAL_STACK_SIZE, NULL, 5, NULL, 1);
    };

    auto drawDeviceMenu = []()
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        clearPage();
        vTaskDelay(pdMS_TO_TICKS(10));
        device->drawDeviceMenu();
    };

    auto drawSearch = []()
    {
        clearScreen();
        xTaskCreatePinnedToCore(drawSearchTask, "drawSearchTask", 15 * configMINIMAL_STACK_SIZE, NULL, 5, NULL, 1);
    };

    auto onDeviceMenuItemSelected = []()
    {
        device->onDeviceMenuItemSelected(currentOption);
    };

    auto startTask = [](auto task, const char *taskName,
                        TaskHandle_t handle, uint8_t size = 10,
                        uint8_t core = 1)
    {
        return [task, taskName, handle, size, core]() mutable
        {
            xTaskCreatePinnedToCore(task, taskName, size * configMINIMAL_STACK_SIZE,
                                    nullptr, 1, &handle, core);
        };
    };

    auto drawActiveMenu = [](const MenuItem *menu, int count)
    {
        return [menu, count]()
        {
            activeMenu = menu;
            activeMenuCount = count;

            rightEncoder.setBoundaries(0, activeMenuCount - 1);
            rightEncoder.setAcceleration(0);
            rightEncoder.setEncoderValue(-1);

            clearScreen();

            if (menuTaskHandle != nullptr)
            {
                // then the menu task is already running, so we don't need to create it again.
                return;
            }
            drawMenu();
        };
    };

    auto espRestart = []()
    {
        esp_restart();
    };

} // namespace actions