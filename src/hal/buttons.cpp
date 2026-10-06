#include "hal/buttons.h"
#include <multi_button.h>

namespace button
{
    static constexpr uint8_t MAX_HANDLERS = 4;
    static constexpr uint16_t LONG_PRESS_DELAY_MS = 3000;

    static Button btn;
    static EventHandler handlers[MAX_HANDLERS];
    static uint8_t handler_count = 0;

    static uint8_t read_gpio(uint8_t btn_id)
    {
        return gpio_get_level(static_cast<gpio_num_t>(btn_id));
    }

    static void dispatch(Button *handle, void *user_data)
    {
        ButtonEvent evt = button_get_event(handle);
        uint8_t state = button_is_pressed(handle) ? 1 : 0;

        for (int i = handler_count - 1; i >= 0; i--)
        {
            if (handlers[i](handle, static_cast<uint8_t>(evt), state))
                return;
        }
    }

    void addHandler(EventHandler h)
    {
        if (handler_count < MAX_HANDLERS)
            handlers[handler_count++] = h;
    }

    void init(void)
    {
        gpio_set_direction(static_cast<gpio_num_t>(PIN_BT0), GPIO_MODE_INPUT);
        gpio_set_pull_mode(static_cast<gpio_num_t>(PIN_BT0), GPIO_PULLUP_ONLY);

        button_init(&btn, read_gpio, 0, PIN_BT0);
        button_attach(&btn, BTN_PRESS_DOWN, dispatch, nullptr);
        button_attach(&btn, BTN_PRESS_UP, dispatch, nullptr);
        button_attach(&btn, BTN_SINGLE_CLICK, dispatch, nullptr);
        button_attach(&btn, BTN_LONG_PRESS_START, dispatch, nullptr);
        button_start(&btn);
    }

    void update(void)
    {
        button_ticks();
    }
}
