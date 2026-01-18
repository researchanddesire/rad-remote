#ifndef MEMORY_SERVICE_H
#define MEMORY_SERVICE_H

#include <Arduino.h>

#include <at24c04.h>

#include "pins.h"

extern bool isMemoryChipFound;

extern AT24C04 memoryService;

bool initMemoryService();

#endif  // MEMORY_SERVICE_H