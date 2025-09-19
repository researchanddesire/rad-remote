#include "search.h"
#include <state/remote.h>

void drawSearchTask(void *pvParameters)
{

    // auto isInCorrectState = []()
    // {
    //     return stateMachine->is("device_search"_s);
    // };

    // if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    // {
    //     tft.fillScreen(ST77XX_BLACK);
    //     tft.setTextSize(1);
    //     tft.setTextColor(ST77XX_WHITE);
    //     tft.setCursor(0, 0);
    //     tft.println("Searching for OSSM");
    //     xSemaphoreGive(displayMutex);
    // }

    // while (isInCorrectState())
    // {
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}

void searchForDevicesTask(void *pvParameters)
{
}