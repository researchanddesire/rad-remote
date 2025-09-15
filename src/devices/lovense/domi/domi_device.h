#ifndef DOMI_DEVICE_H
#define DOMI_DEVICE_H

#include "../../device.hpp"
#include <ArduinoJson.h>

class Domi2 : public Device
{
public:
    String serviceUUID = "0000fff0-0000-1000-8000-00805f9b34fb";
    Domi2();
    virtual ~Domi2();

private:
    void initializeCharacteristics();
    void initializeSettings();
};

#endif // DOMI_DEVICE_H
