#include "wsclient.h"

namespace wsclient
{
    void init(void)
    {
        // device_id: 芯片 MAC 前 3 字节
        char path[64];
        snprintf(path, sizeof(path), "%s?device_id=%06x", WS_SERVER_PATH, (uint32_t)(ESP.getEfuseMac() >> 40));
        Serial.printf("[wsclient] path = %s\n", path);

        ws.setReconnectInterval(1000);
        ws.enableHeartbeat(4000, 12000, 0); // 心跳 4 秒一次，12 秒没响应就断线重连
        ws.onEvent(on_event);
        ws.begin(WS_SERVER_HOST, WS_SERVER_PORT, path);
    }

    void update(void)
    {
        if (isServerConnected && millis() - lastAliveMs >= ALIVE_MS)
        {
            ws.sendTXT(R"({"type":"pub_alive"})");
            lastAliveMs = millis();
        }
        ws.loop();
    }

    void send_message(const String &message)
    {
        ws.sendTXT(message.c_str(), message.length());
    }

    void on_message(void (*callback)(const String &message))
    {
        onMessageCallback = callback;
    }
}
