#pragma once
#include <stdint.h>
#include "pins.h"

namespace buzzer {
    void init(void);
    void on(uint16_t frequency, uint16_t durationMs = 0);
    void on_when(uint16_t frequency, uint16_t delayMs = 0);
    void on_for(uint16_t frequency, uint16_t durationMs, uint16_t delayMs = 0);
    void off(void);
    extern uint8_t volume;

    inline void tap_low()
    {
        on(1000, 15);
    }

    inline void tap_high()
    {
        on(3000, 15);
    }
}