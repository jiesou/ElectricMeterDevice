#include "hal/time_ntp.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>

namespace
{
    constexpr const char *NTP_SERVER = "ntp.aliyun.com";

    bool started = false;
    bool synced = false;

    void on_sync(struct timeval *)
    {
        synced = true;
    }
}

namespace time_ntp
{
    // SNTP 在后台任务里收发，这里只负责起服务；WiFi 没连上时它自己会失败重试
    void init(void)
    {
        esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, NTP_SERVER);
        sntp_set_time_sync_notification_cb(on_sync);
        esp_sntp_init();
        started = true;
    }

    void update(void)
    {
        if (!synced || WiFi.status() == WL_CONNECTED)
            return;

        // WiFi 掉线时 SNTP 会停在旧状态，重连后重启它再对一次
        synced = false;
        Serial.println("[ntp] wifi lost, restart sntp");
        esp_sntp_restart();
    }

    uint32_t now(void)
    {
        return time(nullptr);
    }
}
