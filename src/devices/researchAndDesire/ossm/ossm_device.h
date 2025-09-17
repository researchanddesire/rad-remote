#ifndef OSSM_DEVICE_H
#define OSSM_DEVICE_H

#include "../../device.hpp"
#include <ArduinoJson.h>

class OSSM : public Device
{
public:
    explicit OSSM(const NimBLEAdvertisedDevice *advertisedDevice) : Device(advertisedDevice) {}

    NimBLEUUID getServiceUUID() override { return NimBLEUUID(OSSM_SERVICE_ID); }
};

#endif // OSSM_DEVICE_H
