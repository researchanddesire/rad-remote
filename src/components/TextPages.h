#ifndef TEXT_PAGES_H
#define TEXT_PAGES_H

#include "Arduino.h"
#include "constants/Strings.h"

struct TextPage
{
    String title;
    String description;
    String qrValue = EMPTY_STRING;

    String leftButtonText = EMPTY_STRING;
    String rightButtonText = EMPTY_STRING;
};

static const TextPage deviceSearchPage = {
    .title = DEVICE_SEARCH_TITLE,
    .description = DEVICE_SEARCH_DESCRIPTION,
    .leftButtonText = CANCEL_STRING,
};

static const TextPage deviceStopPage = {.title = DEVICE_STOP_TITLE,
                                        .description = DEVICE_STOP_DESCRIPTION,
                                        .leftButtonText = GO_BACK,
                                        .rightButtonText = GO_HOME};

static const TextPage wifiSettingsPage = {.title = WIFI_SETTINGS_TITLE,
                                          .description = WIFI_SETTINGS_DESCRIPTION,
                                          .qrValue = WIFI_SETTINGS_QR_VALUE,
                                          .leftButtonText = GO_BACK};

#endif // TEXT_PAGES_H