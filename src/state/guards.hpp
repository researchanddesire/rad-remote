#include "events.hpp"
#include "dependencies.hpp"

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
