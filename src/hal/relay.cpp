#include "hal/relay.h"

namespace relay
{
    // 上电就吸合：先写好电平再开输出，免得开输出那一刻抖一下
    void init(void)
    {
        for (const auto &[id, pin] : ENTITY_RELAY_MAP)
        {
            gpio_set_level(pin, 1);
            gpio_set_direction(pin, GPIO_MODE_OUTPUT);
        }
    }
}
