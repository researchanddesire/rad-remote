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
        // start 'drawControlTask'
        xTaskCreatePinnedToCore(drawControllerTask, "drawControllerTask", 15 * configMINIMAL_STACK_SIZE, NULL, 5, NULL, 0);
    };

} // namespace actions