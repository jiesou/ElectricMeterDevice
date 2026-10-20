#pragma once
#include <stdint.h>
#include <deque>
#include <optional>
#include <vector>

// 一条记录只填它真有的字段，没填的字段不会进报文
struct Entity
{
    const char *id;
    const char *type; // "switch" / "meter"
    std::optional<const char *> name;
    std::optional<bool> state;
    std::optional<float> powerW;
    std::optional<float> energyKwh;
    std::optional<uint64_t> ts;
};

namespace entities
{
    extern std::vector<Entity> data;
    extern std::deque<Entity> queue;
    extern int peopleCount; // 房间人数，服务器推下来的净数，大屏说多少就是多少

    void init(void);
    void update(void);
}
