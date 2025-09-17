#ifndef TEXT_PAGES_H
#define TEXT_PAGES_H

#include "Arduino.h"
#include "constants/Strings.h"
#include <optional>

struct TextPage
{
    String title;
    String description;
};

static const TextPage deviceSearchPage = {.title = DEVICE_SEARCH_TITLE,
                                      .description = DEVICE_SEARCH_DESCRIPTION};

#endif // TEXT_PAGES_H