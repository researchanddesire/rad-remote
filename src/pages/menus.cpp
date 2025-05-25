#include "menus.h"
#include "services/display.h"
#include <services/encoder.h>
#include <state/remote.h>

TaskHandle_t menuTaskHandle = NULL;

const MenuItem *activeMenu = mainMenu;
int activeMenuCount = numMainMenu;
int currentOption = 0;

static const int scrollWidth = 6;
static const int scrollHeight = Display::HEIGHT - Display::StatusbarHeight;
static GFXcanvas16 *scrollCanvas = nullptr;

using namespace sml;

static void drawScrollBar(int currentOption, int numOptions)
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

static const int menuWidth = Display::WIDTH - scrollWidth - Display::Padding::P1 * 2;
static const int menuItemHeight = Display::Icons::Small + Display::Padding::P2;
static GFXcanvas16 menuItemCanvas(menuWidth, menuItemHeight);

static void drawMenuItem(int index, const MenuItem &option, bool selected = false)
{
    auto text = option.name.c_str();
    auto bitmap = option.bitmap;
    auto color = option.color > 0 ? option.color : Colors::white;
    auto unfocusedColor = option.unfocusedColor > 0 ? option.unfocusedColor : Colors::bgGray600;

    int y = Display::StatusbarHeight + Display::Padding::P2 + (menuItemHeight)*index;
    menuItemCanvas.fillScreen(Colors::black);

    if (index > 0)
    {
        menuItemCanvas.drawFastHLine(Display::Padding::P1, 0,
                                     menuWidth - Display::Padding::P2,
                                     Colors::bgGray900);
    }

    if (selected)
    {
        menuItemCanvas.fillRoundRect(0, 0, menuWidth, menuItemHeight, 3, Colors::bgGray900);
    }

    menuItemCanvas.setTextColor(selected ? color : unfocusedColor);
    menuItemCanvas.setFont(&FreeSans9pt7b);

    int padding = Display::Padding::P2;
    int textOffset = 6;

    menuItemCanvas.drawBitmap(padding, textOffset, bitmap,
                              Display::Icons::Small, Display::Icons::Small,
                              selected ? color : unfocusedColor);

    padding += Display::Icons::Small + Display::Padding::P2;
    menuItemCanvas.setCursor(padding, textOffset + menuItemHeight / 2);
    menuItemCanvas.print(text);

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        tft.drawRGBBitmap(Display::Padding::P1, y,
                          menuItemCanvas.getBuffer(), menuWidth,
                          menuItemHeight);
        xSemaphoreGive(displayMutex);
    }
}

void drawMenu()
{
    int rawCurrentOption = currentOption;
    int numOptions = activeMenuCount;
    const MenuItem *options = activeMenu;

    int currentOption = abs(rawCurrentOption % numOptions);
    if (rawCurrentOption < 0 && currentOption != 0)
    {
        currentOption = numOptions - currentOption;
    }

    if (numOptions <= 5)
    {
        for (int i = 0; i < numOptions; i++)
        {
            const MenuItem &option = options[i];
            bool isSelected = i == currentOption;
            drawMenuItem(i, option, isSelected);
        }
        return;
    }

    drawScrollBar(currentOption, numOptions - 1);

    for (int i = 0; i < 5; i++)
    {
        int optionIndex = currentOption - 2 + i;
        if (optionIndex < 0)
        {
            optionIndex = numOptions + optionIndex;
        }
        else if (optionIndex > numOptions - 1)
        {
            optionIndex = optionIndex - numOptions;
        }

        const MenuItem &option = options[optionIndex];
        bool isSelected = optionIndex == currentOption;
        drawMenuItem(i, option, isSelected);
    }
}

void drawMenuTask(void *pvParameters)
{
    int lastEncoderValue = -1;
    std::string lastState = "";
    String lastStateName = "";

    auto isInCorrectState = []()
    {
        return stateMachine->is("main_menu"_s) || stateMachine->is("settings_menu"_s);
    };

    while (isInCorrectState())
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);

        String stateName = "";
        stateMachine->visit_current_states(
            [&](auto state)
            { stateName = state.c_str(); });

        currentOption = rightEncoder.readEncoder();

        if (lastEncoderValue == currentOption && stateName == lastStateName)
        {
            continue;
        }
        else
        {
            lastEncoderValue = currentOption;
            lastStateName = stateName;
            drawMenu();
        }
    }

    menuTaskHandle = nullptr;

    vTaskDelete(NULL);
}