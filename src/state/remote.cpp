#include "remote.h"

SettingPercents settings = {
    .speed = 0,
    .stroke = 0,
    .sensation = 0,
    .depth = 0,
    .pattern = StrokePatterns::SimpleStroke,
    .speedKnob = 0};

SettingPercents lastSettings = {
    .speed = 0,
    .stroke = 0,
    .sensation = 0,
    .depth = 0,
    .pattern = StrokePatterns::SimpleStroke,
    .speedKnob = 0};

sender s{};

StateLogger stateLogger;
// Static pointer to hold the state machine instance
sml::sm<ossm_remote_state, sml::thread_safe<ESP32RecursiveMutex>, sml::logger<StateLogger>> *stateMachine = nullptr;

void initStateMachine()
{
    if (stateMachine == nullptr)
    {
        stateMachine = new sml::sm<ossm_remote_state, sml::thread_safe<ESP32RecursiveMutex>, sml::logger<StateLogger>>(s, stateLogger);

        stateMachine->process_event(done{});
    }
}