#include "memory.h"

AT24C04 memoryService(AT24C_ADDRESS_0);

bool isMemoryChipFound = false;

bool initMemoryService() {
    Wire.begin();

    uint8_t testValue = 0xA5; // arbitrary test value
    uint8_t backup = memoryService.read(0);

    memoryService.write(0, testValue);
    delay(5); // short delay for eeprom write

    uint8_t verify = memoryService.read(0);

    if (verify == testValue) {
        ESP_LOGI("MEMORY", "Memory chip found");
        isMemoryChipFound = true;
        // restore original value
        memoryService.write(0, backup);
        delay(5);
    } else {
        ESP_LOGI("MEMORY", "Memory chip not found");
    }


    return true;
}