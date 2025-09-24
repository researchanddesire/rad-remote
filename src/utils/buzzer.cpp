#include "buzzer.h"

#include <unordered_map>
#include <vector>
// Task handle for the buzzer pattern
static TaskHandle_t buzzerTaskHandle = NULL;
static bool buzzerActive = false;
static BuzzerPattern currentPattern = BuzzerPattern::NONE;

struct Beat {
    int frequency;
    int duration;
};

std::unordered_map<BuzzerPattern, std::vector<Beat>> PATTERN_MAP = {
    {BuzzerPattern::MARIO_COIN, {{2637, 100}, {3136, 150}}},
    {BuzzerPattern::SINGLE_BEEP, {{1000, 100}}},
    {BuzzerPattern::DOUBLE_BEEP, {{1000, 100}, {1000, 100}}},
    {BuzzerPattern::TRIPLE_BEEP, {{1000, 100}, {1000, 100}, {1000, 100}}},
    {BuzzerPattern::ERROR_BEEP, {{300, 100}}},
};

void buzzerTask(void *parameter) {
    while (true) {
        if (currentPattern == BuzzerPattern::NONE) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Mario coin two-note motif: E7, G7 (approx). Adjust as desired.
        const std::vector<Beat> &beats = PATTERN_MAP[currentPattern];
        for (const Beat &beat : beats) {
            tone(pins::BUZZER_PIN, beat.frequency);
            vTaskDelay(pdMS_TO_TICKS(beat.duration));
            noTone(pins::BUZZER_PIN);
            vTaskDelay(1);
        }

        currentPattern = BuzzerPattern::NONE;
    }

    vTaskDelete(NULL);
}

void initBuzzer() {
    pinMode(pins::BUZZER_PIN, OUTPUT);
    digitalWrite(pins::BUZZER_PIN, LOW);
    playBuzzerPattern(BuzzerPattern::SINGLE_BEEP);

    xTaskCreatePinnedToCore(buzzerTask,                    // Task function
                            "BuzzerTask",                  // Task name
                            3 * configMINIMAL_STACK_SIZE,  // Stack size
                            nullptr,                       // Parameter
                            1,                             // Priority
                            &buzzerTaskHandle,             // Task handle
                            0);
}

void playBuzzerPattern(BuzzerPattern pattern) { currentPattern = pattern; }

void stopBuzzer() {
    if (buzzerActive && buzzerTaskHandle != NULL) {
        vTaskDelete(buzzerTaskHandle);
        buzzerTaskHandle = NULL;
        digitalWrite(pins::BUZZER_PIN, LOW);
        buzzerActive = false;
    }
}

// Simple Mario coin jingle using Arduino tone API on the direct buzzer pin
void playMarioCoin() {
    // Mario coin two-note motif: E7, G7 (approx). Adjust as desired.
    const int notes[] = {2637, 3136};
    const int durations[] = {100, 150};
    const size_t count = sizeof(notes) / sizeof(notes[0]);
    for (size_t i = 0; i < count; ++i) {
        tone(pins::BUZZER_PIN, notes[i]);
        vTaskDelay(pdMS_TO_TICKS(durations[i]));
        noTone(pins::BUZZER_PIN);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}