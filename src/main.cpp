#include <Arduino.h>

#include <OneButton.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "components/AnimatedIcons.h"
#include "constants.h"
#include "esp_log.h"
#include "pins.h"
#include "services/battery.h"
#include "services/buzzer.h"
#include "services/coms.h"
#include "services/display.h"
#include "services/encoder.h"
#include "services/imu.h"
#include "services/leds.h"
#include "services/vibrator.h"
#include "services/wm.h"
#include "state/remote.h"

OneButton leftShoulderBtn;
OneButton rightShoulderBtn;
OneButton underLeftBtn;
OneButton underCenterBtn;
OneButton underRightBtn;

void setup() {
    Serial.begin(115200);

    // Version 1.x of the PCB Boards cannot use PSRAM
    if (psramInit()) {
        ESP_LOGI(TAG, "PSRAM initialized");
    } else {
        ESP_LOGI(TAG, "PSRAM initialization failed");
    }

    // init buttons
    leftShoulderBtn = OneButton(pins::BTN_L_SHOULDER, true, true);
    leftShoulderBtn.attachClick(
        []() { stateMachine->process_event(left_shoulder_pressed()); });
    rightShoulderBtn = OneButton(pins::BTN_R_SHOULDER, true, true);
    rightShoulderBtn.attachClick(
        []() { stateMachine->process_event(right_shoulder_pressed()); });
    underLeftBtn = OneButton(pins::BTN_UNDER_L, true, true);
    underLeftBtn.attachClick(
        []() { stateMachine->process_event(left_button_pressed()); });
    underCenterBtn = OneButton(pins::BTN_UNDER_C, true, true);
    underCenterBtn.attachClick(
        []() { stateMachine->process_event(middle_button_pressed()); });
    underRightBtn = OneButton(pins::BTN_UNDER_R, true, true);
    underRightBtn.attachClick(
        []() { stateMachine->process_event(right_button_pressed()); });

    initEncoderService();

    initFastLEDs();
    initWM();
    initDisplay();
    initBuzzer();
    initVibrator();
    initBLE();
    initStateMachine();

    initBattery();

    // initIMUService();
    // updateIMUReadings();

    setupAnimatedIcons();

    xTaskCreatePinnedToCore(
        [](void *pvParameters) {
            while (true) {
                vTaskDelay(10);
                leftShoulderBtn.tick();
                rightShoulderBtn.tick();
                underLeftBtn.tick();
                underCenterBtn.tick();
                underRightBtn.tick();
                wm.process();
            }
        },
        "buttonTask", 4 * configMINIMAL_STACK_SIZE, NULL,
        configMAX_PRIORITIES - 1, NULL, 0);
}

void loop() {
    // delete the loop task. Everything is managed by the state machine now.
    vTaskDelete(NULL);
}
