#pragma once
#include <Arduino.h>
#include <WebSocketsClient.h>
#include "firmware_config.h"

namespace wsclient
{
    inline bool isServerConnected = false;

    inline WebSocketsClient ws;
    inline void (*onMessageCallback)(const String &message) = nullptr;
    inline String fragmentBuffer;
    inline uint32_t lastAliveMs = 0;
    constexpr uint32_t ALIVE_MS = 3 * 1000; // 协议定的应用层心跳间隔

    inline void on_event(WStype_t type, uint8_t *payload, size_t length)
    {
        switch (type)
        {
        case WStype_CONNECTED:
            Serial.println("[wsclient] Connected");
            isServerConnected = true;
            lastAliveMs = millis() - ALIVE_MS; // 连上立刻对一次时，别等 3 秒
            break;
        case WStype_DISCONNECTED:
            Serial.println("[wsclient] Disconnected");
            isServerConnected = false;
            break;
        case WStype_TEXT:
            if (onMessageCallback)
                onMessageCallback(String((char *)payload));
            break;
        case WStype_FRAGMENT_TEXT_START:
            fragmentBuffer = String((char *)payload);
            break;
        case WStype_FRAGMENT:
            fragmentBuffer += String((char *)payload);
            break;
        case WStype_FRAGMENT_FIN:
            fragmentBuffer += String((char *)payload);
            if (onMessageCallback)
                onMessageCallback(fragmentBuffer);
            fragmentBuffer = "";
            break;
        }
    }

    void init(void);
    void update(void);
    void send_message(const String &message);
    void on_message(void (*callback)(const String &message));
}
