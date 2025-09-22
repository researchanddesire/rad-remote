#include <Arduino.h>
#include "controller.h"
#include <components/TextButton.h>
#include <constants.h>
#include <state/remote.h>
#include <services/encoder.h>
#include <components/Image.h>
#include <devices/researchAndDesire/ossm/ossm_device.hpp>
#include <components/LinearRailGraph.h>

using namespace sml;
void drawControllerTask(void *pvParameters)
{

    Device *device = static_cast<Device *>(pvParameters);

    // wait until the device is connected
    while (!device->isConnected)
    {
        // TODO: UI / UX here.
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    device->displayObjects.clear();
    // give other tasks a chance to catch up.
    vTaskDelay(100 / portTICK_PERIOD_MS);
    device->drawControls();

    // // Top bumpers
    // TextButton topLeftBumper("<-", pins::BTN_L_SHOULDER, 0, 0);
    // TextButton topRightBumper("->", pins::BTN_R_SHOULDER, DISPLAY_WIDTH - 60, 0);

    // // Bottom bumpers
    // TextButton bottomLeftBumper("Home", pins::BTN_UNDER_L, 0, DISPLAY_HEIGHT - 25);
    // TextButton bottomRightBumper("Patterns", pins::BTN_UNDER_R, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT - 25);

    // TextButton centerButton("STOP", pins::BTN_UNDER_C, DISPLAY_WIDTH / 2 - 60, DISPLAY_HEIGHT - 25, 120);

    // LinearRailGraph linearRail(&ossm->settings.stroke, &ossm->settings.depth, -1, Display::PageHeight - 40, Display::WIDTH);

    // // Create a left encoder dial with Speed parameter
    // std::map<String, float *> leftParams = {
    //     {"Speed", &ossm->settings.speed}};

    // EncoderDial leftDial(leftParams, "", true, 0, Display::PageY + 10);

    // // Create a right encoder dial with all parameters
    // std::map<String, float *> rightParams = {
    //     {"Stroke", &ossm->settings.stroke},
    //     {"Depth", &ossm->settings.depth},
    //     {"Sens.", &ossm->settings.sensation}};

    // EncoderDial rightDial(rightParams, "", false, DISPLAY_WIDTH - 90, Display::PageY + 10);

    // leftEncoder.setBoundaries(0, 100);
    // leftEncoder.setAcceleration(50);
    // leftEncoder.setEncoderValue(device->settings.speed);

    // rightEncoder.setBoundaries(0, 100);
    // rightEncoder.setAcceleration(50);
    // rightEncoder.setEncoderValue(device->settings.stroke);

    bool lastLeftShoulderState = HIGH;
    bool lastRightShoulderState = HIGH;
    bool currentLeftShoulderState = HIGH;
    bool currentRightShoulderState = HIGH;

    int lastLeftEncoderValue = -1;
    int lastRightEncoderValue = -1;
    int currentLeftEncoderValue = -1;
    int currentRightEncoderValue = -1;

    auto isInCorrectState = []()
    {
        return stateMachine->is("device_draw_control"_s);
    };

    while (isInCorrectState())
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

        // Encoders
        currentLeftEncoderValue = leftEncoder.readEncoder();
        currentRightEncoderValue = rightEncoder.readEncoder();

        if (currentLeftEncoderValue != lastLeftEncoderValue)
        {
            device->onLeftEncoderChange();
            lastLeftEncoderValue = currentLeftEncoderValue;
        }
        if (currentRightEncoderValue != lastRightEncoderValue)
        {
            device->onRightEncoderChange();
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

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    // unique_ptr will clean up automatically when the device is destroyed or vector cleared
    device->displayObjects.clear();

    vTaskDelete(NULL);
}

void drawStopTask(void *pvParameters)
{

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        tft.fillScreen(ST77XX_RED);

        // Draw stop sign octagon
        int centerX = tft.width() / 2;
        int centerY = tft.height() / 2;
        int radius = min(tft.width(), tft.height()) / 3;

        // Draw octagon - rotated 22.5 degrees (PI/8) to make top flat
        for (int i = 0; i < 8; i++)
        {
            float angle1 = (i * PI / 4) + (PI / 8);
            float angle2 = ((i + 1) * PI / 4) + (PI / 8);
            int x1 = centerX + radius * cos(angle1);
            int y1 = centerY + radius * sin(angle1);
            int x2 = centerX + radius * cos(angle2);
            int y2 = centerY + radius * sin(angle2);
            // Draw multiple lines with slight offsets to create thicker lines
            for (int offset = -1; offset <= 1; offset++)
            {
                tft.drawLine(x1 + offset, y1, x2 + offset, y2, ST77XX_WHITE);
                tft.drawLine(x1, y1 + offset, x2, y2 + offset, ST77XX_WHITE);
            }
        }

        // Draw text
        tft.setTextSize(2);
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextWrap(false);

        // Center text
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds("EMERGENCY", 0, 0, &x1, &y1, &w, &h);
        tft.setCursor(centerX - w / 2, centerY - h - 5);
        tft.println("EMERGENCY");

        tft.getTextBounds("STOP", 0, 0, &x1, &y1, &w, &h);
        tft.setCursor(centerX - w / 2, centerY + 5);
        tft.println("STOP");

        // Draw instruction text below stop sign
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE); // High contrast color
        tft.setTextWrap(true);

        // Calculate position for instruction text
        int instructionY = centerY + radius + 10; // Position below the stop sign
        tft.setCursor(5, instructionY);
        tft.println("You've emergency stopped your OSSM. Restart your controller and your OSSM to continue");

        xSemaphoreGive(displayMutex);
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
    vTaskDelete(NULL);
}

void drawPatternMenuTask(void *pvParameters)
{

    auto isInCorrectState = []()
    {
        return stateMachine->is("device_menu"_s);
    };

    Image image(0, 0, 64, 64);

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(0, 0);
        tft.println("Pattern Menu");
        xSemaphoreGive(displayMutex);
    }

    image.draw();

    while (isInCorrectState())
    {
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(0, 0);
        xSemaphoreGive(displayMutex);
    }

    image.~Image();

    vTaskDelete(NULL);
}
