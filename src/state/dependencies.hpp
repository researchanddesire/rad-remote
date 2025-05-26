#pragma once

#include "esp_log.h"
#include "events.hpp"

struct sender
{

    void send(const base_event &msg) { ESP_LOGI("SENDER", "send: %d", msg.id); }
};