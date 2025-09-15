#include "ossm_device.h"

OSSM::OSSM() : Device(1024)
{
    serviceUUID = "522B443A-4F53-534D-0001-420BADBABE69";
    deviceName = "OSSM";
}

OSSM::~OSSM()
{
    if (characteristics != nullptr)
    {
        delete[] characteristics;
    }
}
