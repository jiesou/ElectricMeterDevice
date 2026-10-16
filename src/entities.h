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
    extern int peopleCount; // 房间人数，服务器推下来的净数，大屏说多少就是多少

    void init(void);
    void update(void);
}
