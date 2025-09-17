#include "events.hpp"
#include "services/encoder.h"

// template <typename Event>
// constexpr bool is_valid(const Event &event)
// {
//     return event.valid;
// }

// template <typename Event>
//
template <typename Event>
const auto is_valid = [](const Event &event)
{
    ESP_LOGI("TEST", "is_valid");
    return true;
};

template <typename Event = right_button_pressed>
auto isOption = [](MenuItemE value)
{
    return [value](const Event &event) -> bool
    {
        auto currentOption = rightEncoder.readEncoder();
        auto indexOfValue = -1;

        for (int i = 0; i < activeMenuCount; i++)
        {
            if (activeMenu[i].id == value)
            {
                indexOfValue = i;
                break;
            }
        }

        bool result = currentOption == indexOfValue;
        return result;
    };
};