#include "vibrator.h"

// Task handle for the vibrator pattern
static TaskHandle_t vibratorTaskHandle = NULL;
static bool vibratorActive = false;

// Internal pattern definitions
const unsigned short SINGLE_PULSE[] = {50};
const unsigned short DOUBLE_PULSE[] = {50, 200, 50};
const unsigned short TRIPLE_PULSE[] = {10, 50, 20, 50, 30};
const unsigned short ERROR_PULSE[] = {500};

// Pattern mapping structure
struct PatternInfo {
    const unsigned short *pattern;
    size_t length;
};

// Pattern lookup table
const PatternInfo PATTERN_MAP[] = {
    {SINGLE_PULSE, sizeof(SINGLE_PULSE) / sizeof(SINGLE_PULSE[0])},
    {DOUBLE_PULSE, sizeof(DOUBLE_PULSE) / sizeof(DOUBLE_PULSE[0])},
    {TRIPLE_PULSE, sizeof(TRIPLE_PULSE) / sizeof(TRIPLE_PULSE[0])},
    {ERROR_PULSE, sizeof(ERROR_PULSE) / sizeof(ERROR_PULSE[0])}};

// Task configuration constants
constexpr uint32_t VIBRATOR_TASK_STACK_SIZE =
    2 * configMINIMAL_STACK_SIZE;  // 2 * minimum stack size (1024)
constexpr UBaseType_t VIBRATOR_TASK_PRIORITY = 1;

void vibratorTask(void *parameter) {
    VibratorPattern pattern = *((VibratorPattern *)parameter);
    delete (VibratorPattern *)parameter;  // Clean up the parameter

    // Get pattern info from lookup table
    const PatternInfo &info = PATTERN_MAP[static_cast<int>(pattern)];
    const unsigned short *durations = info.pattern;
    size_t length = info.length;

    // Play the pattern
    for (size_t i = 0; i < length; i++) {
        if (i % 2 == 0) {
            // Even indices are vibration durations
            digitalWrite(pins::VIBRATOR, HIGH);
            vTaskDelay(pdMS_TO_TICKS(durations[i]));
        } else {
            // Odd indices are pause durations
            digitalWrite(pins::VIBRATOR, LOW);
            vTaskDelay(pdMS_TO_TICKS(durations[i]));
        }
    }

    // Ensure vibrator is off at the end
    digitalWrite(pins::VIBRATOR, LOW);

    // pause for 100ms between patterns
    vTaskDelay(pdMS_TO_TICKS(100));

    vibratorActive = false;
    vibratorTaskHandle = NULL;
    vTaskDelete(NULL);
}

void initVibrator() { pinMode(pins::VIBRATOR_PIN, OUTPUT); }

void playVibratorPattern(VibratorPattern pattern) {
    // If a pattern is already playing, stop it
    if (vibratorActive && vibratorTaskHandle != NULL) {
        vTaskDelete(vibratorTaskHandle);
        vibratorTaskHandle = NULL;
        vibratorActive = false;
    }

    // Create new parameter for the task
    VibratorPattern *patternParam = new VibratorPattern(pattern);
    if (patternParam == nullptr) {
        return;  // Memory allocation failed
    }

    // Create the task
    BaseType_t taskCreated =
        xTaskCreatePinnedToCore(vibratorTask,              // Task function
                                "VibratorTask",            // Task name
                                VIBRATOR_TASK_STACK_SIZE,  // Stack size
                                patternParam,              // Parameter
                                VIBRATOR_TASK_PRIORITY,    // Priority
                                &vibratorTaskHandle,       // Task handle
                                0);

    if (taskCreated != pdPASS) {
        delete patternParam;  // Clean up if task creation failed
        return;
    }

    vibratorActive = true;
}

void stopVibrator() {
    // if no task is running, just set the mcp to low JUST to be safe.
    if (vibratorTaskHandle == NULL) {
        vibratorTaskHandle = NULL;
        digitalWrite(pins::VIBRATOR, LOW);
        vibratorActive = false;
    }
}