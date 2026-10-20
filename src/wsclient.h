#pragma once
#include <WebSocketsClient.h>

namespace wsclient
{
    extern volatile bool isServerConnected;

    void init(void);
    bool send_message(const String &message);
    void on_message(void (*callback)(const String &message));
}
