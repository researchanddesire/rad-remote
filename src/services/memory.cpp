#include "memory.h"

AT24C04 memoryService(AT24C_ADDRESS_0);

bool isMemoryChipFound = false;

bool initMemoryService() {
    ESP_LOGI("MEMORY", "Waiting 10 seconds...");
    Wire.begin();

    uint8_t testValue = 0xA5;  // arbitrary test value
    memoryService.write(0, testValue);
    delay(5);  // short delay for eeprom write
    uint8_t verify = memoryService.read(0);
    delay(5);

    if (verify == testValue) {
        isMemoryChipFound = true;
        ESP_LOGI("MEMORY", "We found the memory!");
    } else {
        ESP_LOGI("MEMORY", "We did not find the memory!");
    }

    return true;
}