#include "events.hpp"
#include "actions.hpp"
#include "guards.hpp"
#include "boost/sml.hpp"
#include "pages/controller.h"

namespace sml = boost::sml;

struct ossm_remote_state
{
    auto operator()() const
    {
        using namespace sml;
        using namespace actions;

        return make_transition_table(
            *"init"_s + event<done> = "device_search"_s,

            "device_search"_s + on_entry<_> / (drawPage(deviceSearchPage)),
            "device_search"_s + event<connected_event> = "device_draw_control"_s,
            "device_search"_s + event<left_button_pressed> = "main_menu"_s,

            "main_menu"_s + on_entry<_> / drawActiveMenu(mainMenu, numMainMenu),
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::DEVICE_SEARCH)] / search = "device_search"_s,
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::SETTINGS)] = "settings_menu"_s,
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::RESTART)] = "restart"_s,
            "main_menu"_s + event<connected_event> / start = "device_draw_control"_s,

            "settings_menu"_s + on_entry<_> / drawActiveMenu(settingsMenu, numSettingsMenu),
            "settings_menu"_s + event<left_button_pressed> = "main_menu"_s,
            "settings_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::BACK)] = "main_menu"_s,
            "settings_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::RESTART)] = "restart"_s,

            "device_draw_control"_s + on_entry<_> / drawControl,
            "device_draw_control"_s + event<right_button_pressed> = "device_menu"_s,
            "device_draw_control"_s + event<left_button_pressed> = "main_menu"_s,
            "device_draw_control"_s + event<middle_button_pressed> = "device_stop"_s,
            "device_draw_control"_s + event<disconnected_event> / disconnect = "main_menu"_s,

            "device_menu"_s + on_entry<_> / drawDeviceMenu,
            "device_menu"_s + event<left_button_pressed> = "device_draw_control"_s,
            "device_menu"_s + event<right_button_pressed> / onDeviceMenuItemSelected = "device_draw_control"_s,
            "device_menu"_s + event<middle_button_pressed> = "device_stop"_s,
            "device_menu"_s + event<disconnected_event> / disconnect = "main_menu"_s,

            "device_stop"_s + on_entry<_> / (drawPage(deviceStopPage), stop),
            "device_stop"_s + event<right_button_pressed> / disconnect = "main_menu"_s,
            "device_stop"_s + event<left_button_pressed> / start = "device_draw_control"_s,
            "device_stop"_s + event<middle_button_pressed> / start = "device_draw_control"_s,
            "device_stop"_s + event<disconnected_event> / disconnect = "main_menu"_s,

            "restart"_s + on_entry<_> / espRestart,
            "restart"_s = X);
    }
};