#include "wm.h"

#include "WiFi.h"
#include "state/remote.h"

WiFiManager wm;

void initWM() {
    WiFi.useStaticBuffers(true);
    WiFi.begin();

    wm.setSaveConfigCallback(
        []() { stateMachine->process_event(wifi_connected()); });
}