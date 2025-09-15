#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>

class Device
{
public:
    // Common properties for all devices
    static String serviceUUID;
    static String deviceName;

    String *characteristics;
    int characteristicsCount;
    JsonObject settings;
    DynamicJsonDocument settingsDoc;

    // Constructor with settings document size parameter
    explicit Device(int settingsDocSize = 1024) : settingsDoc(settingsDocSize) {}

    // Virtual destructor for proper cleanup
    virtual ~Device() = default;

    // Virtual methods that child classes can optionally override
    virtual void onRightBumperClick() {}
    virtual void onLeftBumperClick() {}
    virtual void onRightButtonClick() {}
    virtual void onLeftButtonClick() {}
    virtual void onExit() {}

    virtual void onRightEncoderChange() {}
    virtual void onLeftEncoderChange() {}

    virtual void drawControls() {}
    virtual void drawMenu() {}

    virtual void pullValue() {}
    virtual void pushValue() {}
};

#endif // DEVICE_H
