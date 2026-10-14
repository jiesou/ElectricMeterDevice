#include "entities.h"
#include <ArduinoJson.h>
#include "hal/ztw_ddsu666.h"
#include "wsclient.h"

namespace
{
    constexpr const char *SWITCH_ID = "kettle";
    constexpr const char *METER_ID = "kettle-meter";

    struct Reading
    {
        bool on;
        float power_w;
        float energy_kwh;
    };

    bool declared = false;
    Reading kettle = {false, 0, 0};
    Reading sent = {false, 0, 0};

    void send(JsonDocument &doc)
    {
        String out;
        serializeJson(doc, out);
        wsclient::send_message(out);
    }

    // 只在读数变了的时候发，没提到的字段服务器保持原样
    void send_entities(bool declare)
    {
        const bool switch_changed = declare || kettle.on != sent.on;
        const bool meter_changed = declare || kettle.power_w != sent.power_w || kettle.energy_kwh != sent.energy_kwh;
        if (!switch_changed && !meter_changed)
            return;

        JsonDocument doc;
        doc["type"] = "pub_entities";
        JsonArray list = doc["entities"].to<JsonArray>();

        if (switch_changed)
        {
            JsonObject e = list.add<JsonObject>();
            e["id"] = SWITCH_ID;
            if (declare)
            {
                e["name"] = "热水壶";
                e["type"] = "switch";
            }
            e["state"] = kettle.on;
        }
        if (meter_changed)
        {
            JsonObject e = list.add<JsonObject>();
            e["id"] = METER_ID;
            if (declare)
            {
                e["name"] = "热水壶功率";
                e["type"] = "meter";
            }
            e["powerW"] = kettle.power_w;
            e["energyKwh"] = kettle.energy_kwh;
        }
        send(doc);
        sent = kettle;
    }

    // 服务器下通断令 {"type":"pub_action_switch","id":"kettle","state":true}
    void on_message(const String &message)
    {
        JsonDocument doc;
        if (deserializeJson(doc, message))
            return;
        if (strcmp(doc["type"] | "", "pub_action_switch") != 0 || strcmp(doc["id"] | "", SWITCH_ID) != 0)
            return;

        kettle.on = doc["state"] | false;
        send_entities(false);

        JsonDocument ack;
        ack["type"] = "ack_action_switch";
        ack["id"] = SWITCH_ID;
        ack["state"] = kettle.on;
        send(ack);
    }

}

namespace entities
{
    void init(void)
    {
        wsclient::on_message(on_message);
    }

    void update(void)
    {
        const uint32_t now = millis();
        static uint32_t lastUpdateMs = 0;
        if (now - lastUpdateMs < 1000)
            return;
        lastUpdateMs = now;

        if (ztw_ddsu666::meters[0].online)
        {
            kettle.power_w = roundf(ztw_ddsu666::meters[0].power_w * 10) / 10;
            kettle.energy_kwh = roundf(ztw_ddsu666::meters[0].energy_kwh * 100) / 100;
        }

        if (!wsclient::isServerConnected)
        {
            declared = false;
            return;
        }
        if (!declared)
        {
            declared = true;
            send_entities(true);
        }
        send_entities(false);
    }
}
