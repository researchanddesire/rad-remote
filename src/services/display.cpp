#include "display.h"

// Screen setup
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, pins::TFT_CS, pins::TFT_DC, pins::TFT_RST);

// Create the display mutex
SemaphoreHandle_t displayMutex = xSemaphoreCreateMutex();

bool initDisplay()
{
    // Initialize screen
    pinMode(pins::TFT_BL, OUTPUT);
    digitalWrite(pins::TFT_BL, HIGH);
    SPI.begin(pins::TFT_SCLK, -1, pins::TFT_MOSI, pins::TFT_CS);
    // Increase SPI frequency for faster display updates (40MHz max for ST7789)
    SPI.setFrequency(40000000); // 40MHz
    tft.init(240, 320); // Initialize with screen dimensions
    tft.setRotation(1); // Landscape mode
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);

    return true;
}
