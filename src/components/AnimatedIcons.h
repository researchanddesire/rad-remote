#ifndef LOCKBOX_ANIMATEDICONS_H
#define LOCKBOX_ANIMATEDICONS_H

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans9pt7b.h> // Using a smaller font

#include <functional>

#include "WiFi.h"
#include "constants/Colors.h"
#include "constants/Sizes.h"
#include "services/encoder.h"
#include <freertos/semphr.h>
#include "components/Icons.h"
#include "services/display.h"
#include "services/lastInteraction.h"
#include "pins.h"
#include <services/battery.h>

class StateIcon
{
protected:
    int16_t x, y;
    int16_t width, height;
    uint16_t color;
    unsigned long lastDrawn = 0;
    GFXcanvas16 *canvas = nullptr;

public:
    bool showStatusDot = false;
    // 0 = off
    // 1 = yellow circle
    // 2 = yellow filled
    // 3 = green circle
    // 4 = green filled
    uint8_t status = 0;

    StateIcon(int16_t x, int16_t y)
        : x(x),
          y(y),
          width(Display::Icons::Small),
          height(Display::Icons::Small),
          color(Colors::white) {}

    void draw(Adafruit_ST7789 *display)
    {
        if (!shouldDraw())
        {
            return;
        }
        canvas = new GFXcanvas16(width, height);
        canvas->drawBitmap(0, 0, getFrame(), width, height, color);

        if (xSemaphoreTake(displayMutex, 100) == pdTRUE)
        {
            display->drawRGBBitmap(x, y, canvas->getBuffer(), width, height);

            if (showStatusDot)
            {
                drawStatusDot(display);
            }
            xSemaphoreGive(displayMutex);
        }

        delete canvas;
        canvas = nullptr;
        lastDrawn = millis();
    }

    void drawStatusDot(Adafruit_ST7789 *display)
    {
        uint size = 2;
        uint padding = 2;
        uint ypadding = 5;

        if (status != 0)
        {
            display->drawCircle(x + padding, y + height - ypadding, size + 1,
                                Colors::bgGray900);
            display->drawCircle(x + padding, y + height - ypadding, size + 2,
                                Colors::bgGray900);
        }

        if (status == 0)
        {
            // Cover up the MQTT circle
            display->fillCircle(x + padding, y + height - ypadding, size,
                                Colors::bgGray600);
        }
        else if (status == 1)
        {
            display->drawCircle(x + padding, y + height - ypadding, size,
                                Colors::yellow);
        }
        else if (status == 2)
        {
            display->fillCircle(x + padding, y + height - ypadding, size,
                                Colors::yellow);
        }
        else if (status == 3)
        {
            display->drawCircle(x + padding, y + height - ypadding, size,
                                Colors::green);
        }
        else if (status == 4)
        {
            display->fillCircle(x + padding, y + height - ypadding, size,
                                Colors::green);
        }
    }

    virtual bool shouldDraw() = 0;
    virtual const unsigned char *getFrame() = 0;
};

class WifiStateIcon : public StateIcon
{
private:
    bool pingOn = false;

public:
    WifiStateIcon(int16_t x, int16_t y) : StateIcon(x, y)
    {
        showStatusDot = true;
    }

    bool shouldDraw() override { return millis() - lastDrawn > 1000; };

    const unsigned char *getFrame() override
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            this->status = 4;
        }
        else
        {
            this->status = 0;
        }

        return WiFiClass::status() == WL_CONNECTED ? bitmap_wifi
                                                   : bitmap_wifi_off;
    }
};

class BatteryStateIcon : public StateIcon
{
public:
    BatteryStateIcon(int16_t x, int16_t y) : StateIcon(x, y) {}

    bool shouldDraw() override { return millis() - lastDrawn > 1000; };

    const unsigned char *getFrame() override
    {
        if (!shouldDraw())
        {
            return nullptr;
        }
        updateBatteryStatus();

        if (isCharging())
        {
            color = Colors::green;
            return bitmap_battery_charging;
        }

        color = Colors::white;
        auto busVoltage = getBatteryVoltage();

        if (busVoltage > 3.9)
        {
            return bitmap_battery_full;
        }
        else if (busVoltage > 3.65)
        {
            return bitmap_battery_mid;
        }
        else if (busVoltage > 3.4)
        {
            return bitmap_battery_low;
        }
        else
        {
            color = Colors::red;
            return bitmap_battery_empty;
        }

        return bitmap_battery_low;
    }
};

// Example usage:
static void setupAnimatedIcons()
{
    auto task = [](void *pvParameters)
    {
        BatteryStateIcon batteryIcon(getIconX(0, 2), 3);
        WifiStateIcon wifiIcon(getIconX(1, 2), 3);

        while (true)
        {
            // If we're sleeping, don't draw anything in the status bar.
            if (idleState != IdleState::NOT_IDLE)
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                continue;
            }

            wifiIcon.draw(&tft);
            batteryIcon.draw(&tft);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    };

    xTaskCreatePinnedToCore(task, "draw_battery", 4 * configMINIMAL_STACK_SIZE,
                            nullptr, tskIDLE_PRIORITY, nullptr, 1);
}

#endif // LOCKBOX_ANIMATEDICONS_H
