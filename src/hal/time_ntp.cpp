#include "hal/time_ntp.h"
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <sys/time.h>

namespace
{
    constexpr const char *NTP_SERVER = "ntp.aliyun.com";
    constexpr uint32_t SYNC_MS = 60 * 1000;

    WiFiUDP udp;
    NTPClient client(udp, NTP_SERVER, 0, SYNC_MS);
}

namespace time_ntp
{
    void init(void)
    {
        client.begin();
    }

    void update(void)
    {
        if (WiFi.status() != WL_CONNECTED)
            return;
        if (!client.update())
            return;

        // 把网络时间写进系统时钟，time(nullptr) 才是真的
        const timeval tv = {.tv_sec = (time_t)client.getEpochTime(), .tv_usec = 0};
        settimeofday(&tv, nullptr);
    }

    uint32_t now(void)
    {
        return time(nullptr);
    }
}
