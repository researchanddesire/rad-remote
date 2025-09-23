#ifndef TEXT_PAGES_H
#define TEXT_PAGES_H

#include "Arduino.h"
#include "constants/Strings.h"

struct TextPage
{
    String title;
    String description;

    String leftButtonText = EMPTY_STRING;
    String rightButtonText = EMPTY_STRING;
};

static const TextPage deviceSearchPage = {.title = DEVICE_SEARCH_TITLE,
                                          .description = DEVICE_SEARCH_DESCRIPTION};

static const TextPage deviceStopPage = {.title = DEVICE_STOP_TITLE,
                                        .description = DEVICE_STOP_DESCRIPTION,
                                        .leftButtonText = GO_BACK,
                                        .rightButtonText = GO_HOME};

#endif // TEXT_PAGES_H