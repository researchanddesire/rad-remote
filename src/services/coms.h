
#ifndef LOCKBOX_COMS_H
#define LOCKBOX_COMS_H

#include <WiFi.h>
#include <esp_now.h>
#include <structs/SettingPercents.h>

void initESPNow();

void sendESPNow(SettingPercents settings);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    uint8_t speed;
    uint8_t stroke;
    uint8_t sens;
    uint8_t depth;
    uint8_t pattern;
} struct_message;

#endif