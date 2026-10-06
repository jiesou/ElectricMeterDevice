#pragma once
#include <WebSocketsClient.h>

namespace wsclient
{
    extern bool isServerConnected;

    void init(void);
    void update(void);
    void send_message(const String &message);
    void on_message(void (*callback)(const String &message));
}
