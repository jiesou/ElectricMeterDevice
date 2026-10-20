#include "wsclient.h"
#include <esp_mac.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "firmware_config.h"

namespace
{
    constexpr uint32_t ALIVE_MS = 3 * 1000; // 协议定的应用层心跳间隔
    constexpr size_t TEXT_MAX = 1024;       // 单条消息上限
    constexpr uint8_t QUEUE_LEN = 4;

    WebSocketsClient ws;
    QueueHandle_t txQueue = nullptr; // 要发的消息，存的就是文本本身
    void (*onMessageCallback)(const String &message) = nullptr;
    String fragmentBuffer = "";
    uint32_t lastAliveMs = 0;

    // 收发都只在这个任务里，网络再慢也卡不到 loop
    void socket_task(void *)
    {
        while (true)
        {
            if (wsclient::isServerConnected && millis() - lastAliveMs >= ALIVE_MS)
            {
                ws.sendTXT(R"({"type":"pub_alive"})");
                lastAliveMs = millis();
            }

            char text[TEXT_MAX];
            while (xQueueReceive(txQueue, text, 0) == pdTRUE)
            {
                Serial.printf("[wsclient] send %u bytes\n", strlen(text));
                ws.sendTXT(text, strlen(text));
            }

            ws.loop();
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }

    void on_event(WStype_t type, uint8_t *payload, size_t length)
    {
        switch (type)
        {
        case WStype_CONNECTED:
            Serial.println("[wsclient] Connected");
            wsclient::isServerConnected = true;
            lastAliveMs = millis() - ALIVE_MS; // 连上立刻对一次时，别等 3 秒
            break;
        case WStype_DISCONNECTED:
            Serial.println("[wsclient] Disconnected");
            wsclient::isServerConnected = false;
            break;
        case WStype_TEXT:
            if (onMessageCallback)
                onMessageCallback(String((char *)payload));
            break;
        case WStype_FRAGMENT_TEXT_START:
            fragmentBuffer = String((char *)payload, length); // 分片消息先攒着
            break;
        case WStype_FRAGMENT:
            fragmentBuffer += String((char *)payload, length);
            break;
        case WStype_FRAGMENT_FIN:
            fragmentBuffer += String((char *)payload, length);
            if (onMessageCallback)
                onMessageCallback(fragmentBuffer);
            fragmentBuffer = "";
            break;
        }
    }
}

namespace wsclient
{
    volatile bool isServerConnected = false;

    void init(void)
    {
        txQueue = xQueueCreate(QUEUE_LEN, TEXT_MAX);

        // device_id: 芯片 MAC 前 3 字节
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char path[64];
        snprintf(path, sizeof(path), "%s?device_id=%02x%02x%02x", WS_SERVER_PATH, mac[0], mac[1], mac[2]);
        Serial.printf("[wsclient] path = %s\n", path);

        ws.setReconnectInterval(1000);
        ws.enableHeartbeat(4000, 12000, 0); // 心跳 4 秒一次，12 秒没响应就断线重连
        ws.onEvent(on_event);
        ws.begin(WS_SERVER_HOST, WS_SERVER_PORT, path);

        // ESP32 双核：Arduino loop 在 core 1，WebSocket 在 core 0
        xTaskCreatePinnedToCore(socket_task, "wsclient", 8192, nullptr, 1, nullptr, 0);
    }

    // 只入队就返回，发不出去也不会卡住调用方
    bool send_message(const String &message)
    {
        char text[TEXT_MAX];
        strlcpy(text, message.c_str(), TEXT_MAX);
        return xQueueSend(txQueue, text, 0) == pdTRUE;
    }

    void on_message(void (*callback)(const String &message))
    {
        onMessageCallback = callback;
    }
}
