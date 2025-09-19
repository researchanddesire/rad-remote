#include "remote.h"

SettingPercents settings = {};

SettingPercents lastSettings = {};

StateLogger stateLogger;
// Static pointer to hold the state machine instance
sml::sm<ossm_remote_state, sml::thread_safe<ESP32RecursiveMutex>, sml::logger<StateLogger>> *stateMachine = nullptr;

void initStateMachine()
{
    if (stateMachine == nullptr)
    {
        stateMachine = new sml::sm<ossm_remote_state, sml::thread_safe<ESP32RecursiveMutex>, sml::logger<StateLogger>>(stateLogger);

        stateMachine->process_event(done{});
    }
}