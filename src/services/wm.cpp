#include "wm.h"

WiFiManager wm;

void initWM()
{

    esp_wifi_set_ps(WIFI_PS_NONE);
    WiFi.begin();
}