#include "buzzer.h"

// Task handle for the buzzer pattern
static TaskHandle_t buzzerTaskHandle = NULL;
static bool buzzerActive = false;

// Pattern timing constants (in milliseconds)
const int BEEP_DURATION = 100;
const int BEEP_GAP = 100;
const int ERROR_BEEP_DURATION = 300;

void buzzerTask(void *parameter)
{
    BuzzerPattern pattern = *((BuzzerPattern *)parameter);
    delete (BuzzerPattern *)parameter; // Clean up the parameter

    switch (pattern)
    {
    case BuzzerPattern::SINGLE_BEEP:
        mcp.digitalWrite(pins::BUZZER, HIGH);
        vTaskDelay(pdMS_TO_TICKS(BEEP_DURATION));
        mcp.digitalWrite(pins::BUZZER, LOW);
        break;

    case BuzzerPattern::DOUBLE_BEEP:
        for (int i = 0; i < 2; i++)
        {
            mcp.digitalWrite(pins::BUZZER, HIGH);
            vTaskDelay(pdMS_TO_TICKS(BEEP_DURATION));
            mcp.digitalWrite(pins::BUZZER, LOW);
            if (i < 1)
                vTaskDelay(pdMS_TO_TICKS(BEEP_GAP));
        }
        break;

    case BuzzerPattern::TRIPLE_BEEP:
        for (int i = 0; i < 3; i++)
        {
            mcp.digitalWrite(pins::BUZZER, HIGH);
            vTaskDelay(pdMS_TO_TICKS(BEEP_DURATION));
            mcp.digitalWrite(pins::BUZZER, LOW);
            if (i < 2)
                vTaskDelay(pdMS_TO_TICKS(BEEP_GAP));
        }
        break;

    case BuzzerPattern::ERROR_BEEP:
        mcp.digitalWrite(pins::BUZZER, HIGH);
        vTaskDelay(pdMS_TO_TICKS(ERROR_BEEP_DURATION));
        mcp.digitalWrite(pins::BUZZER, LOW);
        break;
    }

    buzzerActive = false;
    buzzerTaskHandle = NULL;
    vTaskDelete(NULL);
}

void initBuzzer()
{
    // Nothing to initialize for now
}

void playBuzzerPattern(BuzzerPattern pattern)
{
    // If a pattern is already playing, stop it
    if (buzzerActive && buzzerTaskHandle != NULL)
    {
        vTaskDelete(buzzerTaskHandle);
        buzzerTaskHandle = NULL;
    }

    // Create new parameter for the task
    BuzzerPattern *patternParam = new BuzzerPattern(pattern);

    // Create the task
    buzzerActive = true;
    xTaskCreate(
        buzzerTask,       // Task function
        "BuzzerTask",     // Task name
        2048,             // Stack size
        patternParam,     // Parameter
        1,                // Priority
        &buzzerTaskHandle // Task handle
    );
}

void stopBuzzer()
{
    if (buzzerActive && buzzerTaskHandle != NULL)
    {
        vTaskDelete(buzzerTaskHandle);
        buzzerTaskHandle = NULL;
        mcp.digitalWrite(pins::BUZZER, LOW);
        buzzerActive = false;
    }
}