#include "hal/server_time.h"
#include <Arduino.h>

namespace
{
    uint64_t base = 0;   // 服务器给的毫秒
    uint32_t baseMs = 0; // 收到那一刻的 millis
}

namespace server_time
{
    // 心跳回包一到就重新对一次，3 秒一次，走偏了也追得回来
    void sync(uint64_t server_ms)
    {
        base = server_ms;
        baseMs = millis();
    }

    uint64_t now(void)
    {
        if (!base)
            return 0; // 还没对上，报文里就不带 ts
        return base + (millis() - baseMs);
    }
}
