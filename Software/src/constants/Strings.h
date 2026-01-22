#ifndef STRINGS_H
#define STRINGS_H

#include "Arduino.h"

static const char DEVICE_SEARCH_TITLE[] PROGMEM = "Device Search";
static const char DEVICE_SEARCH_DESCRIPTION[] PROGMEM =
    "Searching for nearby devices...";

static const char DEVICE_STOP_TITLE[] PROGMEM = "Device Stopped";
static const char DEVICE_STOP_DESCRIPTION[] PROGMEM =
    "Your device has been stopped and reset to default play settings; but it's "
    "still connected.";

static const char GO_BACK[] PROGMEM = "Back";
static const char GO_HOME[] PROGMEM = "Home";
static const char CANCEL_STRING[] PROGMEM = "Cancel";

// MENUS

static const char EMPTY_STRING[] PROGMEM = "";
static const char DEFAULT_OSSM_PATTERN_NAME[] PROGMEM = "Simple Stroke";

static const char OSSM_CONTROLLER_NAME[] PROGMEM = "Device Search";
static const char SETTINGS_NAME[] PROGMEM = "Settings";
static const char SLEEP_NAME[] PROGMEM = "Sleep";

static const char GO_BACK_NAME[] PROGMEM = "Go Back";
static const char WIFI_SETTINGS_NAME[] PROGMEM = "WiFi Settings";
static const char PAIRING_NAME[] PROGMEM = "Pairing";
static const char UPDATE_NAME[] PROGMEM = "Update Device";
static const char RESTART_NAME[] PROGMEM = "Restart Device";
static const char DEEP_SLEEP_NAME[] PROGMEM = "Sleep";

static const char WIFI_SETTINGS_TITLE[] PROGMEM = "WiFi Settings";
static const char WIFI_SETTINGS_DESCRIPTION[] PROGMEM =
    "Join the network called 'OSSM Remote Setup' to configure WiFi on this "
    "device.";
static const char WIFI_SETTINGS_QR_VALUE[] PROGMEM =
    "WIFI:S:OSSM Remote Setup;T:nopass;;";

static const char WIFI_CONNECTED_TITLE[] PROGMEM = "Wi-Fi Connected";
static const char WIFI_CONNECTED_DESCRIPTION[] PROGMEM =
    "Your OSSM Remote is now connected to WiFi.";
#endif