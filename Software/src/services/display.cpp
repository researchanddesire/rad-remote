#include "display.h"
#include "esp_log.h"

static const char* TAG = "DISPLAY";

// PWM configuration for backlight
static const int BACKLIGHT_PWM_CHANNEL = 0;
static const int BACKLIGHT_PWM_FREQ = 5000;      // 5kHz - high enough to avoid flicker
static const int BACKLIGHT_PWM_RESOLUTION = 8;   // 8-bit resolution (0-255)

// Track current brightness for restore functionality
static uint8_t currentBrightness = BRIGHTNESS_FULL;

// Screen setup
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, pins::TFT_CS, pins::TFT_DC, pins::TFT_RST);

// Create the display mutex
SemaphoreHandle_t displayMutex = xSemaphoreCreateMutex();

bool initDisplay()
{
    // Initialize PWM for backlight control
    ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RESOLUTION);
    ledcAttachPin(pins::TFT_BL, BACKLIGHT_PWM_CHANNEL);
    ledcWrite(BACKLIGHT_PWM_CHANNEL, BRIGHTNESS_FULL);
    currentBrightness = BRIGHTNESS_FULL;
    ESP_LOGI(TAG, "Backlight PWM initialized on channel %d", BACKLIGHT_PWM_CHANNEL);

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

void setScreenBrightness(uint8_t brightness)
{
    ledcWrite(BACKLIGHT_PWM_CHANNEL, brightness);
    currentBrightness = brightness;
    ESP_LOGD(TAG, "Screen brightness set to %d", brightness);
}

void dimScreen()
{
    setScreenBrightness(BRIGHTNESS_DIM);
}

void restoreScreenBrightness()
{
    setScreenBrightness(BRIGHTNESS_FULL);
}
