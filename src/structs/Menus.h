#ifndef LOCKBOX_MENUITEM_H
#define LOCKBOX_MENUITEM_H

#include <Arduino.h>

#include <vector>

#include "components/Icons.h"
#include "constants/Strings.h"
// numeric enum for every menu item ever.
enum MenuItemE {
    DEVICE_SEARCH,
    SETTINGS,
    SLEEP,
    RESTART,
    BACK,
    WIFI_SETTINGS,
    PAIRING,
    UPDATE,
    DEVICE_MENU_ITEM
};

struct MenuItem {
    const MenuItemE id;
    const std::string name;
    const uint8_t *bitmap;
    const std::optional<std::string> description = std::nullopt;

    // optional color defaults to -1;
    int color = -1;
    // optional unfocused color defaults to -1;
    int unfocusedColor = -1;

    // optional meta data index. Useful for dynamic device menus.
    int metaIndex = -1;
};

// MainMenu

static std::vector<MenuItem> mainMenu = {
    {MenuItemE::DEVICE_SEARCH, OSSM_CONTROLLER_NAME, researchAndDesireWaves},
    {MenuItemE::SETTINGS, SETTINGS_NAME, bitmap_settings}};

static const int numMainMenu = mainMenu.size();

// SettingsMenu

static std::vector<MenuItem> settingsMenu = {
    {MenuItemE::BACK, GO_BACK_NAME, bitmap_back},
    {MenuItemE::WIFI_SETTINGS, WIFI_SETTINGS_NAME, bitmap_wifi},
    // {MenuItemE::PAIRING, PAIRING_NAME, bitmap_link},
    {MenuItemE::UPDATE, UPDATE_NAME, bitmap_update},
    {MenuItemE::RESTART, RESTART_NAME, bitmap_restart},
};

static const int numSettingsMenu = settingsMenu.size();

#endif