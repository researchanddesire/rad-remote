#include <Arduino.h>
#include "controller.h"
#include <components/TextButton.h>
#include <constants.h>
#include <state/remote.h>
#include <services/encoder.h>
#include <services/lastInteraction.h>
#include <components/Image.h>
#include <devices/researchAndDesire/ossm/ossm_device.hpp>
#include <components/LinearRailGraph.h>

using namespace sml;
void drawControllerTask(void *pvParameters)
{
    clearPage();
    ESP_LOGI(TAG, "IN THE CONTROL TASK");

    Device *device = static_cast<Device *>(pvParameters);

    // wait until the device is connected
    ESP_LOGI(TAG, "Waiting for device to connect");
    while (!device->isConnected)
    {
        // TODO: UI / UX here.
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    device->displayObjects.clear();
    // give other tasks a chance to catch up.
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Clear only the page area, preserving top 30px for status icons
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        tft.fillRect(0, Display::PageY, Display::WIDTH, Display::PageHeight + 32, ST77XX_BLACK);
        xSemaphoreGive(displayMutex);
    }

    device->drawControls();

    bool lastLeftShoulderState = HIGH;
    bool lastRightShoulderState = HIGH;
    bool currentLeftShoulderState = HIGH;
    bool currentRightShoulderState = HIGH;

    int lastLeftEncoderValue = -1;
    int currentLeftEncoderValue = -1;

    int lastRightEncoderValue = -1;
    int currentRightEncoderValue = -1;

    auto isInCorrectState = []()
    {
        return stateMachine->is("device_draw_control"_s);
    };

    while (isInCorrectState() && device != nullptr)
    {
        currentLeftShoulderState = digitalRead(pins::BTN_L_SHOULDER);
        currentRightShoulderState = digitalRead(pins::BTN_R_SHOULDER);

        if (currentLeftShoulderState == LOW && lastLeftShoulderState == HIGH)
        {
            device->onLeftBumperClick();
        }
        if (currentRightShoulderState == LOW && lastRightShoulderState == HIGH)
        {
            device->onRightBumperClick();
        }


        // check if device has persistent left encoder monitoring enabled
        //  if it does, we do not read the left encoder here, as it is being
        //  monitored by the leftEncoderMonitor task
        if (!device->needsPersistentLeftEncoderMonitoring()) {
            currentLeftEncoderValue = leftEncoder.readEncoder();
            if (currentLeftEncoderValue != lastLeftEncoderValue) {
                setNotIdle("left_encoder");
                device->onLeftEncoderChange(currentLeftEncoderValue);
                lastLeftEncoderValue = currentLeftEncoderValue;
            }
        }

        currentRightEncoderValue = rightEncoder.readEncoder();

        if (currentRightEncoderValue != lastRightEncoderValue)
        {
            setNotIdle("right_encoder");
            device->onRightEncoderChange(currentRightEncoderValue);
            lastRightEncoderValue = currentRightEncoderValue;
        }

        // Store current states for next iteration
        lastLeftShoulderState = currentLeftShoulderState;
        lastRightShoulderState = currentRightShoulderState;

        for (auto &displayObject : device->displayObjects)
        {
            displayObject->tick();
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }

                vTaskDelay(16 / portTICK_PERIOD_MS); // ~60fps for smooth updating.

    }

    // unique_ptr will clean up automatically when the device is destroyed or vector cleared
    if (device != nullptr)
    {
        device->displayObjects.clear();
    }

    vTaskDelete(NULL);
}
