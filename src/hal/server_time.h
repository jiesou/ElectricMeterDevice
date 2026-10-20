#pragma once
#include <stdint.h>

namespace server_time
{
    void sync(uint64_t server_ms);
    uint64_t now(void);
}
