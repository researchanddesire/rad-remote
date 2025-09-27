#pragma once
#include <stdlib.h>
struct base_event {
    virtual ~base_event() = default;
    int id{};
};

struct ack : public base_event {
    bool valid{};
};

struct fin : public base_event {
    bool valid{};
};

struct release : public base_event {};

struct timeout : public base_event {};

struct left_shoulder_pressed : public base_event {};

struct right_shoulder_pressed : public base_event {};

struct left_button_pressed : public base_event {};

struct wifi_connected : public base_event {};

struct right_button_pressed : public base_event {};

struct middle_button_pressed : public base_event {};

struct done : public base_event {};

struct connected_event : public base_event {};

struct connected_error_event : public base_event {};

struct disconnected_event : public base_event {};

struct left_encoder_changed : public base_event {
    int value{};
};