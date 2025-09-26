#include "menus.h"

#include <services/encoder.h>
#include <state/remote.h>

#include "displayUtils.h"
#include "services/display.h"

TaskHandle_t menuTaskHandle = NULL;
static volatile bool menuTaskExitRequested = false;

std::vector<MenuItem> *activeMenu = &mainMenu;
int activeMenuCount = numMainMenu;
int currentOption = 0;

static const int scrollWidth = 6;

using namespace sml;

static const int menuWidth =
    Display::WIDTH - scrollWidth - Display::Padding::P1 * 2;
static const int menuItemHeight = Display::Icons::Small + Display::Padding::P2;
static const int menuItemDescriptionHeight = menuItemHeight * 1.5;

static int menuYOffset = 0;

static void drawMenuItem(int index, const MenuItem &option,
                         bool selected = false) {
    auto text = option.name;
    auto bitmap = option.bitmap;
    auto color = option.color > 0 ? option.color : Colors::textForeground;
    auto unfocusedColor = option.unfocusedColor > 0 ? option.unfocusedColor
                                                    : Colors::textBackground;

    int y = Display::StatusbarHeight + Display::Padding::P1 + menuYOffset;
    int x = Display::Padding::P1;

    bool shouldDrawDescription = option.description.has_value() && selected;

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Clear menu item area
        tft.fillRect(x, y, menuWidth, menuItemHeight, Colors::black);

        if (index > 0) {
            tft.drawFastHLine(x + Display::Padding::P1, y,
                              menuWidth - Display::Padding::P2,
                              Colors::bgGray900);
        }

        if (selected) {
            tft.fillRoundRect(x, y, menuWidth,
                              shouldDrawDescription ? menuItemDescriptionHeight
                                                    : menuItemHeight,
                              3, Colors::bgGray900);
        }

        tft.setTextColor(selected ? color : unfocusedColor);
        tft.setFont(&FreeSans9pt7b);

        int padding = Display::Padding::P2;
        int textOffset = 6;

        tft.drawBitmap(x + padding, y + textOffset, bitmap,
                       Display::Icons::Small, Display::Icons::Small,
                       selected ? color : unfocusedColor);

        padding += Display::Icons::Small + Display::Padding::P2;
        tft.setCursor(x + padding, y + textOffset + menuItemHeight / 2);
        tft.print(text.c_str());

        if (shouldDrawDescription) {
            tft.setFont();
            tft.setTextColor(Colors::textForegroundSecondary);

            wrapText(tft, option.description.value().c_str(),
                     {.x = x + Display::Padding::P2,
                      .y = y + textOffset + Display::Icons::Small +
                           Display::Padding::P0,
                      .rightPadding = Display::Padding::P3});
        }

        menuYOffset +=
            shouldDrawDescription ? menuItemDescriptionHeight : menuItemHeight;

        xSemaphoreGive(displayMutex);
    }
}

void drawMenuFrame() {
    int rawCurrentOption = currentOption;
    int numOptions = activeMenuCount;
    const MenuItem *options = activeMenu->data();

    int currentOption = abs(rawCurrentOption % numOptions);
    if (rawCurrentOption < 0 && currentOption != 0) {
        currentOption = numOptions - currentOption;
    }

    menuYOffset = 0;

    if (numOptions <= 5) {
        for (int i = 0; i < numOptions; i++) {
            const MenuItem &option = options[i];
            bool isSelected = i == currentOption;
            drawMenuItem(i, option, isSelected);
        }
        return;
    }

    drawScrollBar(currentOption, numOptions - 1);

    for (int i = 0; i < 5; i++) {
        int optionIndex = currentOption - 2 + i;
        if (optionIndex < 0) {
            optionIndex = numOptions + optionIndex;
        } else if (optionIndex > numOptions - 1) {
            optionIndex = optionIndex - numOptions;
        }

        const MenuItem &option = options[optionIndex];
        bool isSelected = optionIndex == currentOption;
        drawMenuItem(i, option, isSelected);
    }
}

void drawMenuTask(void *pvParameters) {
    int lastEncoderValue = -1;

    auto isInCorrectState = []() {
        return stateMachine->is("main_menu"_s) ||
               stateMachine->is("settings_menu"_s) ||
               stateMachine->is("device_menu"_s);
    };

    // Ensure global handle is set for lifecycle coordination
    menuTaskHandle = xTaskGetCurrentTaskHandle();

    while (isInCorrectState() && !menuTaskExitRequested) {
        vTaskDelay(1);

        currentOption = rightEncoder.readEncoder();

        if (lastEncoderValue == currentOption) {
            continue;
        } else {
            lastEncoderValue = currentOption;
            drawMenuFrame();
        }
    }

    // Mark as finished before self-delete so creator can proceed safely
    menuTaskHandle = NULL;
    vTaskDelete(NULL);
}

void drawMenu() {
    rightEncoder.setBoundaries(0, activeMenuCount - 1, true);
    rightEncoder.setAcceleration(0);
    rightEncoder.setEncoderValue(currentOption % activeMenuCount);

    // If an existing task is running, request cooperative exit and wait
    if (menuTaskHandle != NULL) {
        menuTaskExitRequested = true;
        // Wait (with timeout) for the task to cleanly exit and release any
        // mutexes
        const TickType_t waitStart = xTaskGetTickCount();
        const TickType_t waitTimeout = pdMS_TO_TICKS(200);
        while (menuTaskHandle != NULL &&
               (xTaskGetTickCount() - waitStart) < waitTimeout) {
            vTaskDelay(1);
        }
        // If still not null after timeout, as a last resort suspend to avoid
        // delete-while-holding-mutex
        if (menuTaskHandle != NULL) {
            vTaskSuspend(menuTaskHandle);
            vTaskDelete(menuTaskHandle);
            menuTaskHandle = NULL;
        }
    }

    // Reset exit flag before creating a new task
    menuTaskExitRequested = false;

    ESP_LOGD("MENU", "Drawing menu");

    clearPage();
    vTaskDelay(50 / portTICK_PERIOD_MS);
    xTaskCreatePinnedToCore(drawMenuTask, "drawMenuTask",
                            5 * configMINIMAL_STACK_SIZE, NULL, 5,
                            &menuTaskHandle, 1);
}