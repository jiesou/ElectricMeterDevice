#pragma once
#include <WebSocketsClient.h>

extern WebSocketsClient ws;
extern void (*onMessageCallback)(const String &message);
extern bool isServerConnected;

void wsclient_init(const char *path);
void wsclient_update();
void wsclient_send_message(const String &message);
void wsclient_on_message(void (*callback)(const String &message));
