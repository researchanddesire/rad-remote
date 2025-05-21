#pragma once

#include "events.hpp"
#include "dependencies.hpp"

namespace actions
{

    auto send_fin = [](sender &s)
    { s.send(fin{}); };

    auto send_ack = [](const base_event &event, sender &s)
    {
        s.send(event);
    };

} // namespace actions