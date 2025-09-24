#include "buzzer.h"

// Task handle for the buzzer pattern
static TaskHandle_t buzzerTaskHandle = NULL;
static bool buzzerActive = false;

// Internal pattern definitions
const unsigned short SINGLE_BEEP[] = {100};
const unsigned short DOUBLE_BEEP[] = {100, 100, 100};
const unsigned short TRIPLE_BEEP[] = {100, 100, 100, 100, 100};
const unsigned short ERROR_BEEP[] = {300};

// Pattern mapping structure
struct PatternInfo {
    const unsigned short *pattern;
    size_t length;
};

// Pattern lookup table
const PatternInfo PATTERN_MAP[] = {
    {SINGLE_BEEP, sizeof(SINGLE_BEEP) / sizeof(SINGLE_BEEP[0])},
    {DOUBLE_BEEP, sizeof(DOUBLE_BEEP) / sizeof(DOUBLE_BEEP[0])},
    {TRIPLE_BEEP, sizeof(TRIPLE_BEEP) / sizeof(TRIPLE_BEEP[0])},
    {ERROR_BEEP, sizeof(ERROR_BEEP) / sizeof(ERROR_BEEP[0])}};

void buzzerTask(void *parameter) {
    BuzzerPattern pattern = *((BuzzerPattern *)parameter);
    delete (BuzzerPattern *)parameter;  // Clean up the parameter

    ESP_LOGV("BUZZER", "Starting buzzer pattern %d", static_cast<int>(pattern));

    // Get pattern info from lookup table
    const PatternInfo &info = PATTERN_MAP[static_cast<int>(pattern)];
    const unsigned short *durations = info.pattern;
    size_t length = info.length;

    // Play the pattern
    for (size_t i = 0; i < length; i++) {
        if (i % 2 == 0) {
            // Even indices are beep durations
            digitalWrite(pins::BUZZER, HIGH);
            vTaskDelay(pdMS_TO_TICKS(durations[i]));
        } else {
            // Odd indices are pause durations
            digitalWrite(pins::BUZZER, LOW);
            vTaskDelay(pdMS_TO_TICKS(durations[i]));
        }
    }

    // Ensure buzzer is off at the end
    digitalWrite(pins::BUZZER, LOW);
    ESP_LOGV("BUZZER", "Buzzer pattern complete");

    buzzerActive = false;
    buzzerTaskHandle = NULL;
    vTaskDelete(NULL);
}

void initBuzzer() {
    // Nothing to initialize for now
}

void playBuzzerPattern(BuzzerPattern pattern) {
    // If a pattern is already playing, stop it
    if (buzzerActive && buzzerTaskHandle != NULL) {
        vTaskDelete(buzzerTaskHandle);
        buzzerTaskHandle = NULL;
    }

    // Create new parameter for the task
    BuzzerPattern *patternParam = new BuzzerPattern(pattern);

    // Create the task
    buzzerActive = true;
    xTaskCreatePinnedToCore(buzzerTask,                    // Task function
                            "BuzzerTask",                  // Task name
                            3 * configMINIMAL_STACK_SIZE,  // Stack size
                            patternParam,                  // Parameter
                            1,                             // Priority
                            &buzzerTaskHandle,             // Task handle
                            0);
}

void stopBuzzer() {
    if (buzzerActive && buzzerTaskHandle != NULL) {
        vTaskDelete(buzzerTaskHandle);
        buzzerTaskHandle = NULL;
        digitalWrite(pins::BUZZER, LOW);
        buzzerActive = false;
    }
}