#include "memory.h"

AT24C04 memoryService(AT24C_ADDRESS_0);

bool isMemoryChipFound = false;

bool initMemoryService() {
    Wire.begin();

    uint8_t byte = memoryService.read(0);

    if (byte == 0xFF) {
        ESP_LOGI("MEMORY", "Memory chip found");
        isMemoryChipFound = true;
    } else {
        ESP_LOGI("MEMORY", "Memory chip not found");
    }

    return true;
}