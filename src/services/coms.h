
#ifndef LOCKBOX_COMS_H
#define LOCKBOX_COMS_H

#include <NimBLEDevice.h>
#include <NimBLEClient.h>
#include <ArduinoJson.h>
#include <structs/SettingPercents.h>
#include "state/remote.h"

// OSSM BLE Service and Characteristic UUIDs
#define OSSM_SERVICE_UUID "522B443A-4F53-534D-0001-420BADBABE69"
#define OSSM_CHARACTERISTIC_UUID "522B443A-4F53-534D-0002-420BADBABE69"
#define OSSM_DEVICE_NAME "OSSM"

// BLE client and connection management
extern NimBLEClient *pClient;
extern bool deviceConnected;
extern bool serviceFound;

// Function declarations
void initBLE();
void scanForOSSM();
void connectToOSSM();
void disconnectFromOSSM();
void sendCommand(const String &command);
void sendSettings(SettingPercents settings);
void readOSSMState();

// Legacy function for backward compatibility
void sendESPNow(SettingPercents settings);

// Mode control commands
void goToMenu();
void goToSimplePenetration();
void goToStrokeEngine();

// Parameter setting commands
void setStroke(uint8_t value);
void setDepth(uint8_t value);
void setSensation(uint8_t value);
void setSpeed(uint8_t value);
void setPattern(uint8_t value);

// Utility functions
auto getOSSMState();
bool isConnectedToOSSM();

void onConnect(NimBLEClient *pClient);
void onDisconnect(NimBLEClient *pClient);
bool onScanResult(const NimBLEAdvertisedDevice *advertisedDevice);

// OSSM state structure to match incoming JSON
struct OSSMState
{
    String state;
    uint8_t speed;
    uint8_t stroke;
    uint8_t sensation;
    uint8_t depth;
    uint8_t pattern;
};

extern OSSMState currentOSSMState;

#endif