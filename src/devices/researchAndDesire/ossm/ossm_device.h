#ifndef OSSM_DEVICE_H
#define OSSM_DEVICE_H

#include "../../device.hpp"
#include <ArduinoJson.h>

class OSSM : public Device
{
public:
    explicit OSSM(const NimBLEAdvertisedDevice *advertisedDevice) : Device(advertisedDevice) {}
};

#endif // OSSM_DEVICE_H
