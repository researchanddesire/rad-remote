#ifndef DYNAMICTEXT_H
#define DYNAMICTEXT_H

#include "DisplayObject.h"
#include "constants/Colors.h"
#include "constants/Strings.h"
#include "services/display.h"

class DynamicText : public DisplayObject {
  private:
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
            tft.getTextBounds(lastValue.c_str(), x, y, &x1, &y1, &textWidth,
                              &textHeight);

            // if x is -1, center the text
            if (x == -1) {
                x = (Display::WIDTH - textWidth) / 2;
            }

            tft.fillRect(x, y, textWidth, textHeight, Colors::black);
            tft.setCursor(x, y);
            tft.print(text.c_str());

            xSemaphoreGive(displayMutex);
        }

        lastValue = text;
    };
};

#endif  // DYNAMICTEXT_H