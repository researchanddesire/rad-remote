#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "events.hpp"
#include "pages/controller.h"
#include "pages/menus.h"
#include "services/encoder.h"
#include "pages/search.h"
#include "components/TextPages.h"
#include <devices/device.h>
#include <pages/displayUtils.h>
#include "components/TextButton.h"

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

    auto disconnect = []()
    {
        if (device == nullptr)
        {
            return;
        }

        device->~Device();

        device = nullptr;
    };

    auto drawPage = [](const TextPage &page)
    {
        return [page]()
        {
            clearScreen();
            TextPage *params = new TextPage{page};
            xTaskCreatePinnedToCore([](void *pvParameters)
                                    {
                                    TextPage *params = static_cast<TextPage *>(pvParameters);
                                    String localTitle = params ? params->title : String("");
                                    String localDescription = params ? params->description : String("");
                                    String localLeftButton = params ? params->leftButtonText : String("");
                                    String localRightButton = params ? params->rightButtonText : String("");
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

                                    // Draw title with large bold font
                                    canvas->setFont(&FreeSansBold12pt7b);
                                    canvas->setTextColor(Colors::white);
                                    
                                    // Get title bounds for centering
                                    int16_t titleX1, titleY1;
                                    uint16_t titleWidth, titleHeight;
                                    canvas->getTextBounds(localTitle.c_str(), 0, 0, &titleX1, &titleY1, &titleWidth, &titleHeight);
                                    
                                    // Center title horizontally
                                    int16_t titleX = (width - titleWidth) / 2;
                                    int16_t titleY = Display::Padding::P3 - titleY1; // Top padding
                                    canvas->setCursor(titleX, titleY);
                                    canvas->print(localTitle);

                                    // Draw description with smaller font
                                    canvas->setFont(&FreeSans9pt7b);
                                    canvas->setTextColor(Colors::lightGray);
                                    
                                    // Get description bounds for centering
                                    int16_t descX1, descY1;
                                    uint16_t descWidth, descHeight;
                                    canvas->getTextBounds(localDescription.c_str(), 0, 0, &descX1, &descY1, &descWidth, &descHeight);
                                    
                                    // Center description horizontally, place below title
                                    int16_t descX = (width - descWidth) / 2;
                                    int16_t descY = titleY + titleHeight + Display::Padding::P2 - descY1;
                                    canvas->setCursor(descX, descY);
                                    canvas->print(localDescription);

                                    // Draw buttons if text is provided (using TextButton styling)
                                    const int buttonWidth = 80;
                                    const int buttonHeight = 30;
                                    const int buttonY = height - buttonHeight - Display::Padding::P2;
                                    
                                    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE)
                                    {
                                        tft.drawRGBBitmap(0, Display::PageY, canvas->getBuffer(), width, height);
                                        xSemaphoreGive(displayMutex);
                                    }
            

                                    delete canvas;
                                    canvas = nullptr;
                                    
                                    // Left button
                                    if (localLeftButton.length() > 0 && localLeftButton != EMPTY_STRING)
                                    {
                                        TextButton *leftButton = new TextButton(localLeftButton, pins::BTN_UNDER_L, 0, DISPLAY_HEIGHT - 25);
                                        leftButton->tick();
                                        delete leftButton;
                                        leftButton = nullptr;
                                    }
                                    
                                    // Right button
                                    if (localRightButton.length() > 0 && localRightButton != EMPTY_STRING)
                                    {
                                        TextButton *rightButton = new TextButton(localRightButton, pins::BTN_UNDER_R, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT - 25);
                                        rightButton->tick();
                                        delete rightButton;
                                        rightButton = nullptr;
                                    }

                                    vTaskDelete(NULL); },
                                    "drawPageTask", 5 * configMINIMAL_STACK_SIZE, params, 5, NULL, 1);
        };
    };

    auto drawControl = []()
    {
        clearPage();
        // TODO: The intention is that the device manages its own controls.
        // device->drawControls();
        xTaskCreatePinnedToCore(drawControllerTask, "drawControllerTask", 20 * configMINIMAL_STACK_SIZE, device, 5, NULL, 1);
    };

    auto search = []()
    {
        NimBLEScan *pScan = NimBLEDevice::getScan();
        pScan->start(0);
    };

    auto stop = []()
    {
        if (device == nullptr)
        {
            return;
        }

        device->onStop();
    };

    auto start = []()
    {
        if (device == nullptr)
        {
            return;
        }
        device->onConnect();
    };

    auto drawDeviceMenu = []()
    {
        clearPage();
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