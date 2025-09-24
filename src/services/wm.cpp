#include "wm.h"

#include "state/remote.h"

WiFiManager wm;

void initWM() {
    esp_wifi_set_ps(WIFI_PS_NONE);
    WiFi.begin();

    wm.setSaveConfigCallback(
        []() { stateMachine->process_event(wifi_connected()); });
}