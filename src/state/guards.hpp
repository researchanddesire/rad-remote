#include "events.hpp"
#include "dependencies.hpp"
#include "services/encoder.h"

// template <typename Event>
// constexpr bool is_valid(const Event &event)
// {
//     return event.valid;
// }

// template <typename Event>
//
template <typename Event>
const auto is_valid = [](const Event &event, sender &s)
{
    ESP_LOGI("TEST", "is_valid");
    return true;
};

template <typename Event = right_button_pressed>
auto isOption = [](MenuItemE value)
{
    return [value](const Event &event, sender &s) -> bool
    {
        auto currentOption = rightEncoder.readEncoder();
        auto indexOfValue = -1;

        ESP_LOGD("GUARDS", "Checking if option %d is selected (current: %d)", value, currentOption);

        for (int i = 0; i < activeMenuCount; i++)
        {
            if (activeMenu[i].id == value)
            {
                indexOfValue = i;
                ESP_LOGD("GUARDS", "Found option %d at index %d", value, i);
                break;
            }
        }

        bool result = currentOption == indexOfValue;
        ESP_LOGD("GUARDS", "Option match result: %d", result);
        return result;
    };
};