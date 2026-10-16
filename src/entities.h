#pragma once
#include <stdint.h>
#include <deque>
#include <vector>

// 一条读数就是一次部分更新，没填的字段不会进报文
struct Entity
{
    const char *id;
    const char *name;
    const char *type; // "switch" / "meter"
    bool state;
    float powerW;
    float energyKwh;
    uint32_t ts;
};

namespace entities
{
    extern std::vector<Entity> data;
    extern std::deque<Entity> queue;

    void init(void);
    void update(void);
}
