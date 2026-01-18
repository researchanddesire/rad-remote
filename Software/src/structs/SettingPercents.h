#ifndef SOFTWARE_SETTINGPERCENTS_H
#define SOFTWARE_SETTINGPERCENTS_H

enum class StrokePatterns
{
    SimpleStroke,
    TeasingPounding,
    RoboStroke,
    HalfnHalf,
    Deeper,
    StopNGo,
    Insist,
};

struct SettingPercents
{
    float speed = 0;
    float stroke = 50;
    float sensation = 50;
    float depth = 10; // default to 10% depth
    StrokePatterns pattern = StrokePatterns::SimpleStroke;
};

#endif // SOFTWARE_SETTINGPERCENTS_H
