#include <Arduino.h>
#include "controller.h"
#include <components/TextButton.h>
#include <constants.h>
#include <components/EncoderDial.h>
#include <state/remote.h>
#include <services/encoder.h>
#include <components/Image.h>

using namespace sml;
void drawControllerTask(void *pvParameters)
{

    // Top bumpers
    TextButton topLeftBumper("<-", pins::LEFT_SHOULDER_BTN, 0, 0);
    TextButton topRightBumper("->", pins::RIGHT_SHOULDER_BTN, DISPLAY_WIDTH - 60, 0);

    // Bottom bumpers
    TextButton bottomLeftBumper("Home", pins::LEFT_BTN, 0, DISPLAY_HEIGHT - 25);
    TextButton bottomRightBumper("Patterns", pins::RIGHT_BTN, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT - 25);

    TextButton centerButton("STOP", pins::CENTER_BTN, DISPLAY_WIDTH / 2 - 60, DISPLAY_HEIGHT - 25, 120);

    // Create a left encoder dial with Speed parameter
    std::map<String, int> leftParams = {
        {"Speed", 0}};

    EncoderDial leftDial(leftParams, "", true, 0, DISPLAY_HEIGHT / 2 - 30);

    // Create a right encoder dial with all parameters
    std::map<String, int> rightParams = {
        {"Stroke", 0},
        {"Depth", 0},
        {"Sens.", 0}};

    EncoderDial rightDial(rightParams, "", false, DISPLAY_WIDTH - 90, DISPLAY_HEIGHT / 2 - 30);

    bool lastLeftShoulderState = HIGH;
    bool lastRightShoulderState = HIGH;
    bool currentLeftShoulderState = HIGH;
    bool currentRightShoulderState = HIGH;

    auto isInCorrectState = []()
    {
        return stateMachine->is("ossm_control"_s);
    };

    while (isInCorrectState())
    {

        settings.speed = 100 - leftEncoder.readEncoder();

        leftDial.setParameter(settings.speed);

        if (rightDial.getFocusedIndex() == 0)
        {
            settings.stroke = 100 - rightEncoder.readEncoder();
            rightDial.setParameter(settings.stroke);
        }
        else if (rightDial.getFocusedIndex() == 1)
        {
            settings.depth = 100 - rightEncoder.readEncoder();
            rightDial.setParameter(settings.depth);
        }
        else if (rightDial.getFocusedIndex() == 2)
        {
            settings.sensation = 100 - rightEncoder.readEncoder();
            rightDial.setParameter(settings.sensation);
        }

        currentLeftShoulderState = mcp.digitalRead(pins::LEFT_SHOULDER_BTN);
        currentRightShoulderState = mcp.digitalRead(pins::RIGHT_SHOULDER_BTN);

        // Check for falling edge (button press) on right shoulder
        if (currentRightShoulderState == LOW && lastRightShoulderState == HIGH)
        {
            // Increment focus
            int newParameter = rightDial.incrementFocus();
            rightEncoder.setEncoderValue(100 - newParameter - 1);
        }

        // Check for falling edge (button press) on left shoulder
        if (currentLeftShoulderState == LOW && lastLeftShoulderState == HIGH)
        {
            // Decrement focus
            int newParameter = rightDial.decrementFocus();
            rightEncoder.setEncoderValue(100 - newParameter - 1);
        }

        // Store current states for next iteration
        lastLeftShoulderState = currentLeftShoulderState;
        lastRightShoulderState = currentRightShoulderState;

        topLeftBumper.tick();
        topRightBumper.tick();

        bottomLeftBumper.tick();
        bottomRightBumper.tick();
        centerButton.tick();

        leftDial.tick();
        rightDial.tick();

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    // call destructor on encoder dial.
    leftDial.~EncoderDial();
    rightDial.~EncoderDial();
    topLeftBumper.~TextButton();
    topRightBumper.~TextButton();
    bottomLeftBumper.~TextButton();
    bottomRightBumper.~TextButton();
    centerButton.~TextButton();

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
        return stateMachine->is("ossm_pattern_menu"_s);
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
