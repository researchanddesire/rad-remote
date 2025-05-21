#pragma once

#include <Arduino.h>
#include "events.hpp"
#include "dependencies.hpp"
#include "pages/controller.h"
namespace actions
{

    auto send_fin = [](sender &s)
    { s.send(fin{}); };

    template <typename T>
    auto send_ack = [](const T &event, sender &s)
    {
        s.send(event);
    };

    auto drawControl = [](sender &s)
    {
        xTaskCreatePinnedToCore(drawControllerTask, "drawControllerTask", 10 * configMINIMAL_STACK_SIZE, NULL, 5, NULL, 1);
    };

    auto drawPatternMenu = [](sender &s)
    {
        xTaskCreatePinnedToCore(drawPatternMenuTask, "drawPatternMenuTask", 10 * configMINIMAL_STACK_SIZE, NULL, 5, NULL, 1);
    };

} // namespace actions