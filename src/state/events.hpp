#pragma once
#include <stdlib.h>
struct base_event
{
    virtual ~base_event() = default;
    int id{};
};

struct ack : public base_event
{
    bool valid{};
};

struct fin : public base_event
{
    bool valid{};
};

struct release : public base_event
{
};

struct timeout : public base_event
{
};