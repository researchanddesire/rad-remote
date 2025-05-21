#include "remote.h"

SettingPercents settings = {
    .speed = 0,
    .stroke = 0,
    .sensation = 0,
    .depth = 0,
    .pattern = StrokePatterns::SimpleStroke,
    .speedKnob = 0};

sender s{};

// Static pointer to hold the state machine instance
static sml::sm<ossm_remote_state> *stateMachine = nullptr;

void initStateMachine()
{
    if (stateMachine == nullptr)
    {
        stateMachine = new sml::sm<ossm_remote_state>(s);
    }
}