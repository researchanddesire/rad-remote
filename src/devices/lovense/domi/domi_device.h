#ifndef DOMI_DEVICE_H
#define DOMI_DEVICE_H

#include "../../device.hpp"
#include <ArduinoJson.h>

class Domi2 : public Device
{
public:
    explicit Domi2(const NimBLEAdvertisedDevice *advertisedDevice) : Device(advertisedDevice) {}
};

#endif // DOMI_DEVICE_H
