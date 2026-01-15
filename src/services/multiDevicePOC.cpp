#include "multiDevicePOC.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <vector>

#include "coms.h"
#include "devices/device.h"
#include "devices/researchAndDesire/ossm/ossm_device.hpp"
#include "devices/lovense/LovenseDevice.hpp"

static const char* TAG_POC = "MULTI_DEVICE_POC";

// Store multiple connected devices
static std::vector<Device*> connectedDevices;
static bool pocRunning = false;
static TaskHandle_t pocTaskHandle = nullptr;

/**
 * Connect to a single device from the discovered list
 * Returns the created Device* or nullptr on failure
 */
static Device* connectToDevice(const DiscoveredDevice& discovered) {
    ESP_LOGI(TAG_POC, "Attempting to connect to: %s", discovered.name.c_str());
    
    // Create the device using its factory
    Device* newDevice = (*discovered.factory)(discovered.advertisedDevice);
    
    if (newDevice == nullptr) {
        ESP_LOGE(TAG_POC, "Failed to create device instance for: %s", discovered.name.c_str());
        return nullptr;
    }
    
    // Wait for connection to complete (device constructor starts connectionTask)
    int timeout = 100; // 10 seconds (100 * 100ms)
    while (!newDevice->isConnected && timeout > 0) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        timeout--;
    }
    
    if (!newDevice->isConnected) {
        ESP_LOGE(TAG_POC, "Connection timeout for: %s", discovered.name.c_str());
        delete newDevice;
        return nullptr;
    }
    
    ESP_LOGI(TAG_POC, "✓ Successfully connected to: %s", discovered.name.c_str());
    return newDevice;
}

/**
 * POC Task - runs the multi-device test sequence
 */
static void multiDevicePOCTask(void* pvParameters) {
    ESP_LOGI(TAG_POC, "========================================");
    ESP_LOGI(TAG_POC, "  MULTI-DEVICE PROOF OF CONCEPT START");
    ESP_LOGI(TAG_POC, "========================================");
    
    // Step 1: Wait for scan results
    ESP_LOGI(TAG_POC, "Step 1: Waiting for device scan to complete...");
    vTaskDelay(6000 / portTICK_PERIOD_MS); // Wait for scan timeout + buffer
    
    auto& devices = getDiscoveredDevices();
    ESP_LOGI(TAG_POC, "Found %d devices", devices.size());
    
    if (devices.size() < 2) {
        ESP_LOGE(TAG_POC, "Need at least 2 devices for POC. Found: %d", devices.size());
        ESP_LOGI(TAG_POC, "Make sure 2 supported devices are powered on and advertising.");
        pocRunning = false;
        vTaskDelete(nullptr);
        return;
    }
    
    // List all found devices
    for (size_t i = 0; i < devices.size(); i++) {
        ESP_LOGI(TAG_POC, "  [%d] %s (RSSI: %d)", i, devices[i].name.c_str(), devices[i].rssi);
    }
    
    // Step 2: Connect to first 2 devices
    ESP_LOGI(TAG_POC, "Step 2: Connecting to first 2 devices...");
    
    // Stop scanning before connecting
    NimBLEDevice::getScan()->stop();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    // Connect to device 0
    Device* device1 = connectToDevice(devices[0]);
    if (device1) {
        connectedDevices.push_back(device1);
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Brief delay between connections
    
    // Connect to device 1
    Device* device2 = connectToDevice(devices[1]);
    if (device2) {
        connectedDevices.push_back(device2);
    }
    
    ESP_LOGI(TAG_POC, "Connected to %d devices", connectedDevices.size());
    
    if (connectedDevices.size() < 2) {
        ESP_LOGE(TAG_POC, "Failed to connect to 2 devices. POC incomplete.");
        stopMultiDevicePOC();
        vTaskDelete(nullptr);
        return;
    }
    
    // Step 3: Test simultaneous control
    ESP_LOGI(TAG_POC, "========================================");
    ESP_LOGI(TAG_POC, "Step 3: Testing simultaneous control...");
    ESP_LOGI(TAG_POC, "========================================");
    
    // Call onConnect on both devices (this initializes them)
    for (auto* dev : connectedDevices) {
        ESP_LOGI(TAG_POC, "Calling onConnect() for: %s", dev->getName());
        dev->onConnect();
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Test pattern: Ramp up, hold, ramp down
    ESP_LOGI(TAG_POC, "Test: Ramping both devices from 0%% to 50%%...");
    
    for (int intensity = 0; intensity <= 50; intensity += 10) {
        ESP_LOGI(TAG_POC, "Setting intensity: %d%%", intensity);
        
        for (auto* dev : connectedDevices) {
            // Check device type and call appropriate method
            const char* name = dev->getName();
            
            if (strcmp(name, "OSSM") == 0) {
                // Cast to OSSM and set speed
                OSSM* ossm = static_cast<OSSM*>(dev);
                ossm->setSpeed(intensity);
                ESP_LOGI(TAG_POC, "  OSSM: setSpeed(%d)", intensity);
            } else {
                // Assume Lovense-type device (no RTTI, so use static_cast)
                // This is safe because we control the device factories
                LovenseDevice* lovense = static_cast<LovenseDevice*>(dev);
                lovense->setVibrate(intensity);
                ESP_LOGI(TAG_POC, "  %s: setVibrate(%d)", name, intensity);
            }
        }
        
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    
    ESP_LOGI(TAG_POC, "Holding at 50%% for 3 seconds...");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    
    ESP_LOGI(TAG_POC, "Test: Ramping both devices down to 0%%...");
    
    for (int intensity = 50; intensity >= 0; intensity -= 10) {
        ESP_LOGI(TAG_POC, "Setting intensity: %d%%", intensity);
        
        for (auto* dev : connectedDevices) {
            const char* name = dev->getName();
            
            if (strcmp(name, "OSSM") == 0) {
                OSSM* ossm = static_cast<OSSM*>(dev);
                ossm->setSpeed(intensity);
            } else {
                LovenseDevice* lovense = static_cast<LovenseDevice*>(dev);
                lovense->setVibrate(intensity);
            }
        }
        
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    
    // Step 4: Report results
    ESP_LOGI(TAG_POC, "========================================");
    ESP_LOGI(TAG_POC, "  MULTI-DEVICE POC COMPLETE!");
    ESP_LOGI(TAG_POC, "========================================");
    ESP_LOGI(TAG_POC, "Results:");
    ESP_LOGI(TAG_POC, "  - Devices connected: %d", connectedDevices.size());
    ESP_LOGI(TAG_POC, "  - BLE clients created: %d", NimBLEDevice::getCreatedClientCount());
    
    for (size_t i = 0; i < connectedDevices.size(); i++) {
        auto* dev = connectedDevices[i];
        ESP_LOGI(TAG_POC, "  - Device %d: %s (connected: %s)", 
                 i, 
                 dev->getName(), 
                 dev->isConnected ? "YES" : "NO");
    }
    
    ESP_LOGI(TAG_POC, "");
    ESP_LOGI(TAG_POC, "POC will keep devices connected.");
    ESP_LOGI(TAG_POC, "Call stopMultiDevicePOC() to disconnect.");
    ESP_LOGI(TAG_POC, "========================================");
    
    // Keep the task alive but idle
    while (pocRunning) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // Periodically log connection status
        static int counter = 0;
        if (++counter >= 10) {
            counter = 0;
            ESP_LOGI(TAG_POC, "Status: %d devices still connected", 
                     NimBLEDevice::getConnectedClients().size());
        }
    }
    
    vTaskDelete(nullptr);
}

void startMultiDevicePOC() {
    if (pocRunning) {
        ESP_LOGW(TAG_POC, "POC already running!");
        return;
    }
    
    pocRunning = true;
    connectedDevices.clear();
    
    ESP_LOGI(TAG_POC, "Starting Multi-Device POC...");
    ESP_LOGI(TAG_POC, "Make sure 2 supported devices are powered on!");
    
    // Start scan first
    ESP_LOGI(TAG_POC, "Starting BLE scan...");
    clearDiscoveredDevices();
    NimBLEDevice::getScan()->start(5000); // 5 second scan
    
    // Create the POC task with plenty of stack
    xTaskCreatePinnedToCore(
        multiDevicePOCTask,
        "multiDevicePOC",
        16 * 1024,  // 16KB stack - plenty for this test
        nullptr,
        1,
        &pocTaskHandle,
        1  // Run on core 1
    );
}

bool isMultiDevicePOCRunning() {
    return pocRunning;
}

void stopMultiDevicePOC() {
    if (!pocRunning) {
        return;
    }
    
    ESP_LOGI(TAG_POC, "Stopping Multi-Device POC...");
    
    pocRunning = false;
    
    // Disconnect and delete all devices
    for (auto* dev : connectedDevices) {
        if (dev) {
            ESP_LOGI(TAG_POC, "Disconnecting: %s", dev->getName());
            delete dev;
        }
    }
    connectedDevices.clear();
    
    ESP_LOGI(TAG_POC, "POC stopped. All devices disconnected.");
}
