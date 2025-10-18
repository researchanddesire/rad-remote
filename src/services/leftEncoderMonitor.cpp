#include "leftEncoderMonitor.h"

#include "devices/device.h"
#include "encoder.h"
#include "state/remote.h"

extern Device *device;

static TaskHandle_t leftEncoderTaskHandle = nullptr;

void leftEncoderMonitorTask(void *pvParameters) {
    leftEncoder.setEncoderValue(0);
    int lastLeftEncoderValue = leftEncoder.readEncoder();  // Initialize to zero
    ESP_LOGI("LeftEncoderMonitor",
             "Starting left encoder monitor task, initial value: %d",
             lastLeftEncoderValue);

    // Function to check if we're in device states
    auto isInDeviceStates = []() -> bool {
        using namespace sml;
        bool inControl = stateMachine->is("device_draw_control"_s);
        bool inMenu = stateMachine->is("device_menu"_s);
        ESP_LOGV("LeftEncoderMonitor", "State check - Control: %s, Menu: %s",
                 inControl ? "true" : "false", inMenu ? "true" : "false");
        return inControl || inMenu;
    };

    while (isInDeviceStates()) {
        int currentLeftEncoderValue = leftEncoder.readEncoder();

        if (currentLeftEncoderValue != lastLeftEncoderValue ||
            hasLeftEncoderChanged(false)) {
            // Send encoder change to device
            if (device != nullptr) {
                device->onLeftEncoderChange(currentLeftEncoderValue);
            } else {
                ESP_LOGW("LeftEncoderMonitor",
                         "Device is null, cannot send encoder change");
            }

            lastLeftEncoderValue = currentLeftEncoderValue;
        }

        vTaskDelay(16 / portTICK_PERIOD_MS);  // ~60fps
    }

    ESP_LOGI("LeftEncoderMonitor", "Exiting left encoder monitor task");

    // Clean up task handle when exiting
    leftEncoderTaskHandle = nullptr;
    vTaskDelete(nullptr);
}

void startLeftEncoderMonitoring() {
    // Kill existing task if it exists
    if (leftEncoderTaskHandle != nullptr) {
        vTaskDelete(leftEncoderTaskHandle);
        leftEncoderTaskHandle = nullptr;
        // Small delay to ensure cleanup completes before creating new task
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    xTaskCreatePinnedToCore(leftEncoderMonitorTask, "leftEncoderMonitor",
                            4 * configMINIMAL_STACK_SIZE, nullptr, 5,
                            &leftEncoderTaskHandle, 1);
}

void stopLeftEncoderMonitoring() {
    if (leftEncoderTaskHandle != nullptr) {
        vTaskDelete(leftEncoderTaskHandle);
        leftEncoderTaskHandle = nullptr;
    }
}