#include "vibrator.h"

#include <unordered_map>
#include <vector>

// Task handle for the vibrator pattern
static TaskHandle_t vibratorTaskHandle = NULL;
static bool vibratorActive = false;
static VibratorPattern currentPattern = VibratorPattern::NONE;

struct Pulse {
    int duration;
    int pause = 0;
};

std::unordered_map<VibratorPattern, std::vector<Pulse>> VIBRATOR_PATTERN_MAP = {
    {VibratorPattern::SINGLE_PULSE, {{50}}},
    {VibratorPattern::DOUBLE_PULSE, {{50, 200}, {50}}},
    {VibratorPattern::TRIPLE_PULSE, {{10, 50}, {20, 50}, {30}}},
    {VibratorPattern::ERROR_PULSE, {{500}}},
    {VibratorPattern::SHUTDOWN, {{150}, {100}, {100}, {100}}},
    {VibratorPattern::DEVICE_CONNECTED, {{100}, {100}, {150}}},
    {VibratorPattern::DEVICE_DISCONNECTED, {{150}, {100}, {100}}},
    {VibratorPattern::PAUSED, {{150}, {150}}},
    {VibratorPattern::PLAY, {{100}, {100}}}};

void vibratorTask(void *parameter) {
    while (true) {
        if (currentPattern == VibratorPattern::NONE) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        const std::vector<Pulse> &pulses = VIBRATOR_PATTERN_MAP[currentPattern];
        for (const Pulse &pulse : pulses) {
            digitalWrite(pins::VIBRATOR_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(pulse.duration));
            digitalWrite(pins::VIBRATOR_PIN, LOW);
            if (pulse.pause > 0) {
                vTaskDelay(pdMS_TO_TICKS(pulse.pause));
            }
        }

        currentPattern = VibratorPattern::NONE;
    }

    vTaskDelete(NULL);
}

void initVibrator() {
    pinMode(pins::VIBRATOR_PIN, OUTPUT);
    digitalWrite(pins::VIBRATOR_PIN, LOW);
    playVibratorPattern(VibratorPattern::SINGLE_PULSE);

    xTaskCreatePinnedToCore(vibratorTask,                  // Task function
                            "VibratorTask",                // Task name
                            3 * configMINIMAL_STACK_SIZE,  // Stack size
                            nullptr,                       // Parameter
                            1,                             // Priority
                            &vibratorTaskHandle,           // Task handle
                            0);
}

void playVibratorPattern(VibratorPattern pattern) { currentPattern = pattern; }
