#include "events.hpp"
#include "actions.hpp"
#include "guards.hpp"
#include "dependencies.hpp"
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
            *"init"_s + event<done> = "main_menu"_s,

            "main_menu"_s + on_entry<_> / drawActiveMenu(mainMenu, numMainMenu),
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::OSSM_CONTROLLER)] = "ossm_control"_s,
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::SETTINGS)] = "settings_menu"_s,
            "main_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::RESTART)] = "restart"_s,

            "settings_menu"_s + on_entry<_> / drawActiveMenu(settingsMenu, numSettingsMenu),
            "settings_menu"_s + event<right_button_pressed>[isOption<>(MenuItemE::BACK)] = "main_menu"_s,

            "ossm_control"_s + on_entry<_> / drawControl,
            "ossm_control"_s + event<right_button_pressed> = "ossm_pattern_menu"_s,
            "ossm_control"_s + event<left_button_pressed> = "main_menu"_s,

            "ossm_pattern_menu"_s + on_entry<_> / drawPatternMenu,
            "ossm_pattern_menu"_s + event<right_button_pressed> = "ossm_control"_s,

            "restart"_s + on_entry<_> / espRestart,
            "restart"_s = X);
    }
};