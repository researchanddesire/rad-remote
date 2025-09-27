#ifndef DYNAMICTEXT_H
#define DYNAMICTEXT_H

#include "DisplayObject.h"
#include "constants/Colors.h"
#include "constants/Strings.h"
#include "services/display.h"

class DynamicText : public DisplayObject {
  private:
    static constexpr const char *TAG = "DynamicText";
    const std::string &text;
    std::string lastValue = EMPTY_STRING;

    // get text bounds
    int16_t x1, y1;
    uint16_t textWidth, textHeight;

  public:
    DynamicText(const std::string &text, int16_t x, int16_t y)
        : DisplayObject(x, y, text.length() * 8, 16), text(text) {
        lastValue = text;
    }

    bool shouldDraw() override { return isFirstDraw || text != lastValue; }

    void draw() override {
        ESP_LOGI(TAG, "Drawing DynamicText: %s", text.c_str());
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            tft.setFont(&FreeSans9pt7b);
            tft.setTextColor(Colors::textBackground);

            // Get bounds for both old and new text
            int16_t oldX1, oldY1, newX1, newY1;
            uint16_t oldWidth, oldHeight, newWidth, newHeight;

            tft.getTextBounds(lastValue.c_str(), x, y, &oldX1, &oldY1,
                              &oldWidth, &oldHeight);
            tft.getTextBounds(text.c_str(), x, y, &newX1, &newY1, &newWidth,
                              &newHeight);

            // Calculate combined clearing area
            int16_t clearX = min(oldX1, newX1);
            int16_t clearY = min(oldY1, newY1);
            uint16_t clearWidth =
                max(oldX1 + oldWidth, newX1 + newWidth) - clearX;
            uint16_t clearHeight =
                max(oldY1 + oldHeight, newY1 + newHeight) - clearY;

            // Clear the combined area
            tft.fillRect(clearX, clearY, clearWidth, clearHeight,
                         Colors::black);

            // Draw the new text
            int drawX = (x == -1) ? (Display::WIDTH - newWidth) / 2 : x;
            tft.setCursor(drawX, y);
            tft.print(text.c_str());

            xSemaphoreGive(displayMutex);
        }

        lastValue = text;
    };
};

#endif  // DYNAMICTEXT_H