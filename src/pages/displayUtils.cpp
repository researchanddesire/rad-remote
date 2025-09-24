#include "displayUtils.h"
#include "services/display.h"

static const int scrollWidth = 6;
static const int scrollHeight = Display::HEIGHT - Display::StatusbarHeight;
static GFXcanvas16 *scrollCanvas = nullptr;

void drawScrollBar(int currentOption, int numOptions)
{
    // Always use direct drawing to avoid memory allocation
    float scrollPercent = (float)currentOption / (numOptions);
    int scrollPosition = scrollPercent * (scrollHeight - 20);

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        tft.fillRect(
            Display::WIDTH - scrollWidth, Display::StatusbarHeight,
            scrollWidth, scrollHeight, Colors::black);

        // Track
        tft.drawFastVLine(
            Display::WIDTH - (scrollWidth / 2), Display::StatusbarHeight + Display::Padding::P0,
            scrollHeight - Display::Padding::P1, Colors::bgGray900);

        // Thumb
        tft.fillRoundRect(
            Display::WIDTH - scrollWidth,
            Display::StatusbarHeight + scrollPosition,
            scrollWidth, 20, 3, Colors::white);
        xSemaphoreGive(displayMutex);
    }
}

void clearScreen()
{
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        tft.fillScreen(Colors::black);
        xSemaphoreGive(displayMutex);
    }
}

void clearPage(bool clearStatusbar)
{
    if (clearStatusbar)
    {
        return clearScreen();
    }

    GFXcanvas16 *canvas = new GFXcanvas16(Display::WIDTH, Display::PageHeight);
    canvas->fillScreen(Colors::black);

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        tft.drawRGBBitmap(0, Display::PageY, canvas->getBuffer(), Display::WIDTH, Display::PageHeight);
        //Also clear top left and top right corners to remove buttons
        tft.fillRect(0, 0, 75, Display::StatusbarHeight, Colors::black);
        tft.fillRect(Display::WIDTH - 75, 0, 75, Display::StatusbarHeight, Colors::black);

        xSemaphoreGive(displayMutex);
    }

    delete canvas;
    canvas = nullptr;
}