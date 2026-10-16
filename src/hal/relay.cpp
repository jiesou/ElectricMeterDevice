#include "hal/relay.h"

namespace relay
{
    // 先给低电平再开输出，免得开输出那一刻抖一下
    void init(void)
    {
        for (gpio_num_t pin : PINS)
        {
            gpio_set_level(pin, 0);
            gpio_set_direction(pin, GPIO_MODE_OUTPUT);
        }
    }
}
