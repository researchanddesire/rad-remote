#pragma once

#include "events.hpp"
#include "dependencies.hpp"

namespace actions
{

    auto send_fin = [](sender &s)
    { s.send(fin{}); };

    template <typename T>
    auto send_ack = [](const T &event, sender &s)
    {
        s.send(event);
    };

} // namespace actions