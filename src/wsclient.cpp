#include "wsclient.h"
#include <esp_mac.h>
#include "firmware_config.h"

namespace
{
    constexpr uint32_t ALIVE_MS = 30 * 1000; // 协议定的应用层心跳间隔
    uint32_t lastAliveMs = 0;

    WebSocketsClient ws;
    void (*onMessageCallback)(const String &message) = nullptr;
    String fragmentBuffer = "";
}

namespace wsclient
{
    bool isServerConnected = false;

    void update(void)
    {
        if (isServerConnected && millis() - lastAliveMs >= ALIVE_MS)
        {
            send_message(R"({"type":"pub_alive"})");
            lastAliveMs = millis();
        }
        ws.loop();
    }

    void send_message(const String &message)
    {
        Serial.println("[wsclient] Sending message: " + message);
        ws.sendTXT(message.c_str(), message.length());
    }

    void init(void)
    {
        // device_id: 芯片 MAC 前 3 字节
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char path[64];
        snprintf(path, sizeof(path), "%s?device_id=esp-%02x%02x%02x", WS_SERVER_PATH, mac[0], mac[1], mac[2]);
        Serial.printf("[wsclient] path = %s\n", path);

        ws.setReconnectInterval(1000);
        ws.enableHeartbeat(4000, 12000, 0); // 设置心跳间隔为 4 秒，超时为 12 秒，断开重试次数不限
        ws.onEvent([](WStype_t type, uint8_t *payload, size_t length)
                   {
        Serial.printf("[wsclient] Event type: %d, length: %d\n", type, length);
        switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("[wsclient] Disconnected");
            isServerConnected = false;
            break;
        case WStype_CONNECTED:
            Serial.println("[wsclient] Connected");
            isServerConnected = true;
            lastAliveMs = millis();
            break;
        case WStype_TEXT:
            if (onMessageCallback) {
                onMessageCallback(String((char *)payload));
            }
            break;
        case WStype_BIN:
            break;
        case WStype_FRAGMENT_TEXT_START:
            fragmentBuffer = String((char *)payload); // 开始收集分片消息
            break;
        case WStype_FRAGMENT:
            fragmentBuffer += String((char *)payload); // 继续收集分片
            break;
        case WStype_FRAGMENT_FIN:
            fragmentBuffer += String((char *)payload); // 添加最后一个分片
            if (onMessageCallback)
            {
                onMessageCallback(fragmentBuffer);
            }
            fragmentBuffer = "";
            break;
        case WStype_FRAGMENT_BIN_START:
            fragmentBuffer = "";
            break;
        case WStype_ERROR:
            Serial.printf("[wsclient] Error, type: %d, length: %d\n", type, length);
            break;
        } });
        ws.begin(WS_SERVER_HOST, WS_SERVER_PORT, path);
    }

    void on_message(void (*callback)(const String &message))
    {
        onMessageCallback = callback;
    }
}
