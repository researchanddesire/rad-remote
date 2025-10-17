#include "actions.hpp"
#include "boost/sml.hpp"
#include "events.hpp"
#include "guards.hpp"
#include "pages/controller.h"

namespace sml = boost::sml;

struct ossm_remote_state {
    auto operator()() const {
        using namespace sml;
        using namespace actions;

        return make_transition_table(
            // clang-format off
            *"init"_s + event<done> = "device_search"_s,

            "device_search"_s + on_entry<_> / (drawPage(deviceSearchPage), []() { setLed(LEDColors::logoBlue, 255, 1500); }),
            "device_search"_s + event<connected_event> = "device_draw_control"_s,
            "device_search"_s + event<left_button_pressed> / disconnect = "main_menu"_s,

            "main_menu"_s + on_entry<_> / drawMainMenu,
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::DEVICE_SEARCH)] / search = "device_search"_s,
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::SETTINGS)] = "settings_menu"_s,
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::DEEP_SLEEP)] = "deep_sleep"_s,
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::RESTART)] = "restart"_s,
            "main_menu"_s + event<connected_event> / start = "device_draw_control"_s,

            "settings_menu"_s + on_entry<_> / drawSettingsMenu,
            "settings_menu"_s + event<left_button_pressed> = "main_menu"_s,
            "settings_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::BACK)] = "main_menu"_s,
            "settings_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::WIFI_SETTINGS)] = "wmConfig"_s,
            "settings_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::RESTART)] = "restart"_s,

            "wmConfig"_s + on_entry<_> / (drawPage(wifiSettingsPage), startWiFiPortal),
            "wmConfig"_s + event<left_button_pressed> = "settings_menu"_s,
            "wmConfig"_s + event<wifi_connected> / drawPage(wifiConnectedPage),
            "wmConfig"_s + boost::sml::on_exit<_> / stopWiFiPortal,

            "device_draw_control"_s + on_entry<_> / drawControl,
            "device_draw_control"_s + event<right_button_pressed>[hasDeviceMenu<>] = "device_menu"_s,
            // TODO: Left Menu button needs a menu behind it, this is disabled and only a placeholder for now.
            "device_draw_control"_s + event<left_button_pressed>[hasDeviceSettingsMenu<>] = "device_menu"_s,
            "device_draw_control"_s + event<middle_button_pressed> / softPause,
            "device_draw_control"_s + event<middle_button_second_press> / stop = "device_stop"_s,
            "device_draw_control"_s + event<disconnected_event> / disconnect = "main_menu"_s,

            "device_menu"_s + on_entry<_> / drawDeviceMenu,
            "device_menu"_s + event<left_button_pressed> = "device_draw_control"_s,
            "device_menu"_s + event<right_button_pressed> / onDeviceMenuItemSelected = "device_draw_control"_s,
            "device_menu"_s + event<middle_button_second_press> / softPause = "device_draw_control"_s,
            "device_menu"_s + event<middle_button_pressed> / softPause = "device_draw_control"_s,
            "device_menu"_s + event<disconnected_event> / disconnect = "main_menu"_s,

            "device_stop"_s + on_entry<_> / (drawPage(deviceStopPage), stop),
            "device_stop"_s + event<right_button_pressed> / disconnect = "main_menu"_s,
            "device_stop"_s + event<left_button_pressed> / start = "device_draw_control"_s,
            "device_stop"_s + event<middle_button_pressed> / start = "device_draw_control"_s,
            "device_stop"_s + event<disconnected_event> / disconnect = "main_menu"_s,

            "restart"_s + on_entry<_> / espRestart,
            "restart"_s = X,

            "deep_sleep"_s + on_entry<_> / enterDeepSleep,
            "deep_sleep"_s + event<wake_up_event> = "device_search"_s);

        // clang-format on
    }
};