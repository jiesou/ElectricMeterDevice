#include "main.h"
#include <Arduino.h>
#include <WiFi.h>
#include <multi_button.h>
#include "entities.h"
#include "hal/buzzer.h"
#include "hal/buzzer_player.h"
#include "hal/buttons.h"
#include "hal/ztw_ddsu666.h"
#include "pins.h"
#include "firmware_config.h"
#include "wsclient.h"

namespace
{
    bool on_button_event(Button *b, uint8_t evt, uint8_t)
    {
        if (evt == BTN_PRESS_DOWN)
        {
            digitalWrite(PIN_LED, digitalRead(PIN_LED) == HIGH ? LOW : HIGH);
            Serial.println("[main] BT0 pressed");
        }
        return true;
    }
}

void setup(void)
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println("[main] ElectricMeter booting");

    pinMode(PIN_LED, OUTPUT);
    // buzzer::init();
    // buzzer_player::init();
    // buzzer_player::tada();
    button::init();
    button::addHandler(on_button_event);
    ztw_ddsu666::init();

    if (WIFI_SSID[0] == '\0' || WIFI_PASSWORD[0] == '\0')
    {
        Serial.println("[wifi] no WIFI_SSID in include/pins.h");
    }
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    // WiFi.setSleep(false);
    // WiFi.setTxPower(WIFI_POWER_19_5dBm);
    Serial.print("[wifi] connecting");
    unsigned long start_wifi_millis = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start_wifi_millis < 5000)
    {
        if (WiFi.status() == WL_CONNECT_FAILED)
        {
            Serial.println("[wifi] connection failed!");
            break;
        }
        delay(50);
        Serial.print(".");
    }
    Serial.println("[wifi] connected");

    wsclient::init();
    entities::init();
}

void loop(void)
{
    button::update();
    ztw_ddsu666::update();
    wsclient::update();
    entities::update();
    delay(10);
}
