/*
  Rui Santos
  Complete project details at
  https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include "coms.h"


// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Create a struct_message called myData
struct_message myData;

SettingPercents lastState;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    ESP_LOGV("COMS", "Last Packet Send Status: %s",
             status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void initESPNow()
{

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        ESP_LOGD("COMS", "Error initializing ESP-NOW");
        return;
    }

    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Transmitted packet
    esp_now_register_send_cb(OnDataSent);

    // Register peer
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        ESP_LOGD("COMS", "Failed to add peer");
        return;
    }

    xTaskCreatePinnedToCore(
        [](void *pvParameters)
        {
            while (true)
            {
                sendESPNow(settings);
                vTaskDelay(
                    10 / portTICK_PERIOD_MS); // 100ms delay between broadcasts
            }
        },
        "espnowTask", 4 * configMINIMAL_STACK_SIZE, nullptr, 1, nullptr, 0);
}

void sendESPNow(SettingPercents settings)
{
    // if NO change in settings vs lastState then return.
    if (lastState.speed == settings.speed &&
        lastState.stroke == settings.stroke &&
        lastState.sensation == settings.sensation &&
        lastState.depth == settings.depth &&
        static_cast<int>(lastState.pattern) ==
            static_cast<int>(settings.pattern))
    {
        return;
    }

    myData.speed = settings.speed;
    myData.stroke = settings.stroke;
    myData.sens = settings.sensation;
    myData.depth = settings.depth;
    myData.pattern = static_cast<int>(settings.pattern);

    // Update lastState
    lastState = settings;

    // Send message via ESP-NOW
    esp_err_t result =
        esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK)
    {
        ESP_LOGV("COMS", "Sent with success");
    }
    else
    {
        ESP_LOGD("COMS", "Error sending the data");
    }
}