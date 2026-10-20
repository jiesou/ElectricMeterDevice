#pragma once
#include <driver/gpio.h>
#include <stddef.h>
#include <string.h>
#include <utility>
#include "pins.h"

// 三路断路器，一路一个 IO，高电平吸合。第 i 项就是第 i 路，pin 和挂在上面的开关
namespace relay
{
    constexpr std::pair<const char *, gpio_num_t> ENTITY_RELAY_MAP[] = {
        {"light", static_cast<gpio_num_t>(PIN_RELAY_1)},
        {"kettle", static_cast<gpio_num_t>(PIN_RELAY_2)},
        {"dryer", static_cast<gpio_num_t>(PIN_RELAY_3)},
    };

    void init(void);

    inline void set(const char *switchId, bool on)
    {
        for (const auto &[id, pin] : ENTITY_RELAY_MAP)
            if (strcmp(id, switchId) == 0)
                gpio_set_level(pin, on);
    }
}
