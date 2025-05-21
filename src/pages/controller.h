#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"

void drawControllerTask(void *pvParameters);

void drawPatternMenuTask(void *pvParameters);

#endif