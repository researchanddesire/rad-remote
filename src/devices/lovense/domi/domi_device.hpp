#ifndef DOMI_DEVICE_H
#define DOMI_DEVICE_H

#include "../../device.h"
#include <ArduinoJson.h>
#include <components/TextButton.h>
#include <components/LinearRailGraph.h>

#define DOMI_CHARACTERISTIC_UUID_COMMAND "57300002-0023-4BD4-BBD5-A6920E4C5653"
#define DOMI_CHARACTERISTIC_UUID_RESPONSE "57300003-0023-4BD4-BBD5-A6920E4C5653"

class Domi2 : public Device
{
public:
    explicit Domi2(const NimBLEAdvertisedDevice *advertisedDevice) : Device(advertisedDevice)
    {
        characteristics = {
            {"command", {NimBLEUUID(DOMI_CHARACTERISTIC_UUID_COMMAND)}},
            {"response", {NimBLEUUID(DOMI_CHARACTERISTIC_UUID_RESPONSE)}},
        };
    }

    NimBLEUUID getServiceUUID() override { return NimBLEUUID(DOMI_SERVICE_ID); }
    const char *getName() override { return "Domi 2"; }

    float vibrateIntensity = 0;
    int leftFocusedIndex = 0;

    void onConnect() override
    {
        setVibrate(0);
        isConnected = true;
    }

    void drawControls() override
    {
        leftEncoder.setBoundaries(0, 16);
        leftEncoder.setAcceleration(0);
        leftEncoder.setEncoderValue(this->vibrateIntensity);

        std::map<String, float *> leftParams = {
            {"Vibrate", &this->vibrateIntensity}};
        draw<EncoderDial>(EncoderDial::Props{
            .encoder = &leftEncoder,
            .parameters = leftParams,
            .focusedIndex = &this->leftFocusedIndex,
            .minValue = 0,
            .maxValue = 16});
    }

    // helper functions
    bool setVibrate(int intensity)
    {
        intensity = constrain(intensity, 0, 16);
        String intensityStr = String(intensity);
        return send("command", String("Vibrate:" + intensityStr + ";").c_str());
    }

    int getBatteryLevel()
    {
        send("command", "Battery;");
        return readInt("response", 0);
    }

    bool setPowerOff()
    {
        isConnected = false;
        return send("command", "PowerOff;");
    }

    bool powerOn()
    {
        return send("command", "PowerOn;");
    }

    void onLeftEncoderChange(int value) override
    {
        setVibrate(value);
    }
};

#endif // DOMI_DEVICE_H
