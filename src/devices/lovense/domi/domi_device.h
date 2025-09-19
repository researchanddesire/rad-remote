#ifndef DOMI_DEVICE_H
#define DOMI_DEVICE_H

#include "../../device.hpp"
#include <ArduinoJson.h>

class Domi2 : public Device
{
public:
    explicit Domi2(const NimBLEAdvertisedDevice *advertisedDevice) : Device(advertisedDevice) {}

    NimBLEUUID getServiceUUID() override { return NimBLEUUID(DOMI_SERVICE_ID); }
    const char *getName() override { return "Domi 2"; }
};

#endif // DOMI_DEVICE_H
