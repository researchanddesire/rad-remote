#include "domi_device.h"

Domi2::Domi2() : Device(1024)
{
    // Initialize Domi-specific service UUID
    serviceUUID = "0000fff0-0000-1000-8000-00805f9b34fb"; // Lovense custom service

    // Initialize characteristics
    initializeCharacteristics();

    // Initialize settings
    initializeSettings();
}

Domi2::~Domi2()
{
    if (characteristics != nullptr)
    {
        delete[] characteristics;
    }
}

void Domi2::initializeCharacteristics()
{
    characteristicsCount = 4;
    characteristics = new String[characteristicsCount];
    characteristics[0] = "0000fff1-0000-1000-8000-00805f9b34fb"; // Command characteristic
    characteristics[1] = "0000fff2-0000-1000-8000-00805f9b34fb"; // Data characteristic
    characteristics[2] = "0000fff3-0000-1000-8000-00805f9b34fb"; // Status characteristic
    characteristics[3] = "0000fff4-0000-1000-8000-00805f9b34fb"; // Battery characteristic
}

void Domi2::initializeSettings()
{
    settingsDoc["vibration"] = 0;
    settingsDoc["rotation"] = 0;
    settingsDoc["pattern"] = "steady";
    settingsDoc["enabled"] = false;
    settingsDoc["battery"] = 0;
    settings = settingsDoc.as<JsonObject>();
}
