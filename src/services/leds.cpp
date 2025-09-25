#include "leds.h"
#include <Arduino.h>

#include <constants/Colors.h>
#include <optional>

#include "esp_log.h"

static const char* ledsTaskName = "ledsTask";

static auto TAG = "LED";

CRGB leds[pins::NUM_LEDS];
float ledPaceSpeedRpm = 60.0;  // Default value, adjust as needed
struct LedState {
    std::optional<double> targetColor = std::nullopt;
    std::optional<double> currentColor = std::nullopt;
    double targetBrightness = 0;
    double currentBrightness = 0;

    double progress = 0.0;
    double startColor = 0;
    double startBrightness = 0;
    long startTime = 0;
    uint16_t duration_ms = 250;
};

LedState ledState;

// ease in out sine
double ease(const double t) { return 0.5 * (1 + sin(3.1415926 * (t - 0.5))); }

void ledsTask(void* pvParameters) {
    auto* state = static_cast<LedState*>(pvParameters);
    bool shouldDraw = true;

    while (true) {
        shouldDraw = state->progress < 1.0 &&
                     (state->currentColor != state->targetColor ||
                      state->currentBrightness != state->targetBrightness);

        if (!shouldDraw) {
            vTaskDelay(250 / portTICK_PERIOD_MS);
            continue;
        }

        if (state->duration_ms == 0) {
            state->progress = 1.0;
        } else {
            state->progress =
                constrain(static_cast<double>(millis() - state->startTime) /
                              static_cast<double>(state->duration_ms),
                          0.0, 1.0);
        }

        const float dir =
            state->startColor - state->targetColor.value_or(0) <= 128 ? 1 : -1;

        const double easeProgress = ease(state->progress);

        state->currentColor =
            (state->startColor) +
            dir * easeProgress *
                (state->targetColor.value_or(0) - state->startColor);

        state->currentBrightness =
            state->startBrightness +
            (state->targetBrightness - state->startBrightness) * easeProgress;

        if (state->progress >= 1.0) {
            state->currentColor = state->targetColor;
            state->currentBrightness = state->targetBrightness;
            state->progress = 1.0;
            state->startColor = state->targetColor.value_or(0);
        }

        leds[0] = CHSV(state->currentColor.value_or(0), 255,
                       state->currentBrightness);
        leds[1] = CHSV(state->currentColor.value_or(0), 255,
                       state->currentBrightness);
        leds[2] = CHSV(state->currentColor.value_or(0), 255,
                       state->currentBrightness);
        FastLED.show();

        vTaskDelay(1);
    }
}

void initFastLEDs() {
    ESP_LOGI(TAG, "Initializing FastLEDs...");
    CFastLED::addLeds<LED_TYPE, pins::LED_PIN, COLOR_ORDER>(leds,
                                                            pins::NUM_LEDS)
        .setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.show();

#ifdef DEBUG_LEDS
    ESP_LOGI(TAG, "DEBUG_LEDS is defined, setting leds to red and green");
    // set the leds to red
    setLed(RED, 255);
    FastLED.show();
    vTaskDelay(1000);
    setLed(GREEN, 255);
    FastLED.show();
    vTaskDelay(1000);
    // then clear
    // setLedOff(leds);
#endif

    ESP_LOGI(TAG, "FastLEDs initialization complete.");

    xTaskCreatePinnedToCore(ledsTask, ledsTaskName,
                            5 * configMINIMAL_STACK_SIZE, &ledState,
                            tskIDLE_PRIORITY, nullptr, 1);

    vTaskDelay(100);
    setLed(LEDColors::logoBlue, 255, 1500);
}

void setLed(uint8_t color_value, const uint8_t brightness,
            const uint16_t duration_ms) {
    if (ledState.targetColor.value_or(0) == color_value &&
        ledState.targetBrightness == brightness) {
        return;
    }

    ledState.targetColor = static_cast<double>(color_value);
    ledState.targetBrightness = static_cast<double>(brightness);
    ledState.duration_ms = duration_ms;
    ledState.progress = 0.0;
    ledState.startColor = ledState.currentColor.value_or(color_value);
    ledState.startBrightness = ledState.currentBrightness;
    ledState.startTime = millis();
}

void setLedOff() {
    ledState.targetColor = std::nullopt;
    ledState.targetBrightness = 0;
    ledState.progress = 0.0;
    ledState.startTime = millis();
    ledState.duration_ms = 0;
}
