#include "displayUtils.h"
#include "services/display.h"

static const int scrollWidth = 6;
static const int scrollHeight = Display::HEIGHT - Display::StatusbarHeight;
static GFXcanvas16 *scrollCanvas = nullptr;

void drawScrollBar(int currentOption, int numOptions)
{
    scrollCanvas = new GFXcanvas16(scrollWidth, scrollHeight);
    float scrollPercent = (float)currentOption / (numOptions);
    int scrollPosition = scrollPercent * (scrollHeight - 20);

    scrollCanvas->fillScreen(Colors::black);
    scrollCanvas->drawFastVLine(scrollWidth / 2, Display::Padding::P0,
                                scrollHeight - Display::Padding::P1,
                                Colors::bgGray900);
    scrollCanvas->fillRoundRect(0, scrollPosition, scrollWidth, 20, 3,
                                Colors::white);

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        tft.drawRGBBitmap(
            Display::WIDTH - scrollWidth, Display::StatusbarHeight,
            scrollCanvas->getBuffer(), scrollWidth, scrollHeight);
        xSemaphoreGive(displayMutex);
    }

    delete scrollCanvas;
    scrollCanvas = nullptr;
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
        xSemaphoreGive(displayMutex);
    }

    delete canvas;
    canvas = nullptr;
}