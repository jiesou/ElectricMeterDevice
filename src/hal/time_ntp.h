#pragma once
#include <stdint.h>

namespace time_ntp
{
    void init(void);
    void update(void);
    uint32_t now(void);
}
