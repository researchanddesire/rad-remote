#ifndef LOCKBOX_MENUITEM_H
#define LOCKBOX_MENUITEM_H

#include "components/Icons.h"

#include <Arduino.h>
// numeric enum for every menu item ever.
enum MenuItemE
{
    OSSM_CONTROLLER,
    SETTINGS,
    SLEEP,
    RESTART,
    BACK,
    WIFI_SETTINGS,
    PAIRING,
    UPDATE
};

struct MenuItem
{
    const MenuItemE id;
    String name;
    const uint8_t *bitmap;

    // optional color defaults to -1;
    int color = -1;
    // optional unfocused color defaults to -1;
    int unfocusedColor = -1;

    // this is session setting key which must be set to true to show this item.
    String setting = "";
};

// MainMenu
static const MenuItem mainMenu[] = {
    {MenuItemE::OSSM_CONTROLLER, "OSSM Controller", researchAndDesireWaves},
    {MenuItemE::SETTINGS, "Settings", bitmap_settings},
    {MenuItemE::SLEEP, "Sleep", bitmap_sleep},
    {MenuItemE::RESTART, "Restart", bitmap_restart},
};

static const int numMainMenu = sizeof(mainMenu) / sizeof(mainMenu[0]);


// SettingsMenu
static const MenuItem settingsMenu[] = {
    {MenuItemE::BACK, "Go Back", bitmap_back},
    {MenuItemE::WIFI_SETTINGS, "WiFi Settings", bitmap_wifi},
    {MenuItemE::PAIRING, "Pairing", bitmap_link},
    {MenuItemE::UPDATE, "Update Device", bitmap_update},
    {MenuItemE::RESTART, "Restart Device", bitmap_restart},
};

static const int numSettingsMenu = sizeof(settingsMenu) / sizeof(settingsMenu[0]);

#endif