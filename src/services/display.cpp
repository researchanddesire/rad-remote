#include "display.h"

// Screen setup
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, pins::TFT_CS, pins::TFT_A0, pins::TFT_RST);

bool initDisplay()
{
    // Initialize screen
    pinMode(pins::TFT_BACKLIGHT, OUTPUT);
    digitalWrite(pins::TFT_BACKLIGHT, HIGH);
    SPI.begin(pins::TFT_SCK, -1, pins::TFT_SDA, -1);
    tft.init(240, 320); // Initialize with screen dimensions
    tft.setRotation(1); // Landscape mode
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);

    return true;
}
