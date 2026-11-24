#include "hal/ztw_ddsu666.h"
#include <driver/gpio.h>

namespace ztw_ddsu666
{
    void init(void)
    {
        gpio_set_level(static_cast<gpio_num_t>(PIN_RS485_EN), 0);
        gpio_set_direction(static_cast<gpio_num_t>(PIN_RS485_EN), GPIO_MODE_OUTPUT);
        Serial2.begin(9600, SERIAL_8N1, PIN_UART2_RX, PIN_UART2_TX);
    }

    void update(void)
    {
        pollData();
    }
}