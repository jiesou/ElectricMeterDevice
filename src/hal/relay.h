#pragma once
#include <driver/gpio.h>
#include <stddef.h>
#include "pins.h"

// 三路断路器，一路一个 IO，高电平吸合
namespace relay
{
    constexpr size_t COUNT = 3;
    constexpr gpio_num_t PINS[COUNT] = {
        static_cast<gpio_num_t>(PIN_RELAY_1),
        static_cast<gpio_num_t>(PIN_RELAY_2),
        static_cast<gpio_num_t>(PIN_RELAY_3),
    };

    void init(void);
    inline void set(size_t i, bool on) { gpio_set_level(PINS[i], on); }
}
