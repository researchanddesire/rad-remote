#include "memory.h"

AT24C04 memoryService(AT24C_ADDRESS_0);
AT24C04 memoryService1(AT24C_ADDRESS_1);
AT24C04 memoryService2(AT24C_ADDRESS_2);
AT24C04 memoryService3(AT24C_ADDRESS_3);

bool isMemoryChipFound = false;

bool initMemoryService() {
    Wire.begin();

    // Array of all memory services to test
    AT24C04* services[] = {&memoryService, &memoryService1, &memoryService2,
                           &memoryService3};
    const char* serviceNames[] = {"memoryService (0)", "memoryService1 (1)",
                                  "memoryService2 (2)", "memoryService3 (3)"};
    uint8_t addresses[] = {AT24C_ADDRESS_0, AT24C_ADDRESS_1, AT24C_ADDRESS_2,
                           AT24C_ADDRESS_3};

    uint8_t testValue = 0xA5;  // arbitrary test value
    AT24C04* realService = nullptr;
    int realServiceIndex = -1;

    // Test each service
    for (int i = 0; i < 4; i++) {
        ESP_LOGI("MEMORY", "Testing %s at address 0x%02X", serviceNames[i],
                 addresses[i]);

        uint8_t backup = services[i]->read(0);
        services[i]->write(0, testValue);
        delay(5);  // short delay for eeprom write

        uint8_t verify = services[i]->read(0);

        if (verify == testValue) {
            ESP_LOGI("MEMORY", "%s is REAL! Address: 0x%02X", serviceNames[i],
                     addresses[i]);
            realService = services[i];
            realServiceIndex = i;
            // restore original value
            services[i]->write(0, backup);
            delay(5);
            isMemoryChipFound = true;
            break;  // Found the real one, stop testing
        } else {
            ESP_LOGI("MEMORY",
                     "%s is not real (verify: 0x%02X, expected: 0x%02X)",
                     serviceNames[i], verify, testValue);
        }
    }

    if (realService != nullptr) {
        ESP_LOGI("MEMORY",
                 "=== REAL MEMORY CHIP FOUND: %s at address 0x%02X ===",
                 serviceNames[realServiceIndex], addresses[realServiceIndex]);
    } else {
        ESP_LOGI("MEMORY", "=== NO REAL MEMORY CHIP FOUND ===");
    }

    return true;
}