#pragma once

#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <constants/Sizes.h>
#include <devices/device.h>
#include <pages/displayUtils.h>
#include <pages/genericPages.h>
#include <qrcode.h>
#include <services/wm.h>

#include "components/TextButton.h"
#include "components/TextPages.h"
#include "events.hpp"
#include "pages/controller.h"
#include "pages/menus.h"
#include "services/encoder.h"

namespace actions {

    auto clearPage = [](bool clearStatusbar = false) {
        // small delay to ensure tasks are finished
        vTaskDelay(50 / portTICK_PERIOD_MS);
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (clearStatusbar) {
                tft.fillRect(0, 0, Display::WIDTH, Display::HEIGHT,
                             Colors::black);
            } else {
                tft.fillRect(0, Display::StatusbarHeight, Display::WIDTH,
                             Display::PageHeight + 32, Colors::black);
                // Also clear top left and top right corners to remove buttons
                tft.fillRect(0, 0, 75, Display::StatusbarHeight, Colors::black);
                tft.fillRect(Display::WIDTH - 75, 0, 75,
                             Display::StatusbarHeight, Colors::black);
            }
            xSemaphoreGive(displayMutex);
        }
    };

    auto clearScreen = []() {
        // small delay to ensure tasks are finished
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            tft.fillScreen(Colors::black);
            xSemaphoreGive(displayMutex);
        }
    };

    auto disconnect = []() {
        if (device != nullptr) {
            device->~Device();

            device = nullptr;
        }

        // and then stop scanning.
        NimBLEScan *pScan = NimBLEDevice::getScan();
        pScan->stop();
    };

    auto drawPage = [](const TextPage &page) {
        // Capture reference to static const object - safe since it lives in
        // flash memory
        return [&page]() {
            clearPage();
            xTaskCreatePinnedToCore(drawPageTask, "drawPageTask",
                                    5 * configMINIMAL_STACK_SIZE,
                                    const_cast<TextPage *>(&page), 5, NULL, 1);
        };
    };

    auto drawControl = []() {
        clearPage();  // Use clearPage to clear only the page area, so the
                      // status icons stick around
        // Defer UI work to its own task on core 1 to avoid
        // heavy display ops inside the state-machine/connection context.
        // Helps resolve RAD-598
        xTaskCreatePinnedToCore(
            [](void *pv) {
                clearPage();  // Use clearPage to clear only the page area, so
                              // the status icons stick around
                // Larger stack for complex UI task
                xTaskCreatePinnedToCore(
                    drawControllerTask, "drawControllerTask",
                    16 * configMINIMAL_STACK_SIZE, device, 5, NULL, 1);
                vTaskDelete(NULL);
            },
            "spawnControllerUI", 6 * configMINIMAL_STACK_SIZE, nullptr, 4,
            nullptr, 1);
    };

    auto search = []() {
        NimBLEScan *pScan = NimBLEDevice::getScan();
        pScan->start(0);
    };

    auto stop = []() {
        if (device == nullptr) {
            return;
        }
        device->onStop();
    };

    auto start = []() {
        if (device == nullptr) {
            return;
        }
        device->onConnect();
    };

    auto drawDeviceMenu = []() { device->drawDeviceMenu(); };

    auto onDeviceMenuItemSelected = []() {
        device->onDeviceMenuItemSelected(currentOption);
    };

    auto drawMainMenu = []() {
        activeMenu = &mainMenu;
        activeMenuCount = numMainMenu;
        clearPage();
        drawMenu();
    };

    auto drawSettingsMenu = []() {
        activeMenu = &settingsMenu;
        activeMenuCount = numSettingsMenu;
        clearPage();
        drawMenu();
    };

    auto espRestart = []() { esp_restart(); };

    auto startWiFiPortal = []() {
        // Give a second for any pending MQTT messages to be sent before
        // disconnecting WiFi Otherwise we lose this state_change message until
        // the device comes back online.
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // disconnect from the network
        WiFi.disconnect(true);

        wm.setConfigPortalBlocking(false);
        wm.setConnectTimeout(30);
        wm.setConnectRetries(5);
        wm.setEnableConfigPortal(true);
        wm.setCleanConnect(true);
        wm.startConfigPortal("OSSM Remote Setup");

        // if the wifi is not currently connected then make a small task the
        // looks for the wifi connection and sends an event.
    };

    auto stopWiFiPortal = []() {
        wm.setConfigPortalBlocking(true);
        wm.stopConfigPortal();
    };

}  // namespace actions