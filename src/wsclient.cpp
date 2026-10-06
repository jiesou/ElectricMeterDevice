#include "wsclient.h"
#include "firmware_config.h"

void wsclient_update()
{
    ws.loop();
}

WebSocketsClient ws;
void (*onMessageCallback)(const String &message) = nullptr;
String fragmentBuffer = "";

void wsclient_send_message(const String &message)
{

    Serial.println("[WSClient] Sending message: " + message);
    ws.sendTXT(message.c_str(), message.length());
}

bool isServerConnected = false;

void wsclient_init(const char *path)
{
    ws.setReconnectInterval(1000);
    ws.enableHeartbeat(4000, 12000, 0); // 设置心跳间隔为 4 秒，超时为 12 秒，断开重试次数不限
    ws.onEvent([](WStype_t type, uint8_t *payload, size_t length)
               {
        Serial.printf("[WSClient] Event type: %d, length: %d\n", type, length);
        switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSClient] Disconnected");
            isServerConnected = false;
            break;
        case WStype_CONNECTED:
            Serial.println("[WSClient] Connected");
            isServerConnected = true;
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
            Serial.printf("[WSClient] Error, type: %d, length: %d\n", type, length);
            break;
        } });
    ws.begin(WS_SERVER_HOST, WS_SERVER_PORT, path);
}

void wsclient_on_message(void (*callback)(const String &message))
{
    onMessageCallback = callback;
}