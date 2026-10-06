#include "entities.h"
#include <ArduinoJson.h>
#include "wsclient.h"

namespace
{
    constexpr uint32_t TICK_MS = 1000;       // 读数刷新与增量上报的节奏
    constexpr uint32_t ALIVE_MS = 30 * 1000; // 协议定的应用层心跳间隔
    constexpr float KETTLE_W = 1600;         // 占位读数：热水壶额定功率

    constexpr const char *SWITCH_ID = "kettle";
    constexpr const char *METER_ID = "kettle-meter";

    struct Reading
    {
        bool on;
        float power_w;
        float energy_kwh;
    };

    bool declared = false;
    uint32_t last_tick_ms = 0;
    uint32_t last_alive_ms = 0;
    Reading kettle = {false, 0, 0};
    Reading sent = {false, 0, 0};
    float energy_sum = 0; // 电量累加值，四舍五入会把它吃掉，报出去的时候再取整

    void send(JsonDocument &doc)
    {
        String out;
        serializeJson(doc, out);
        wsclient_send_message(out);
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

    void tick(void)
    {
        const uint32_t now = millis();
        if (now - last_tick_ms < TICK_MS)
            return;

        // 占位读数：开着就是额定功率，电量按流过的时间累加
        const float seconds = (now - last_tick_ms) / 1000.0f;
        kettle.power_w = kettle.on ? KETTLE_W : 0;
        energy_sum += kettle.power_w * seconds / 3.6e6f;
        kettle.energy_kwh = roundf(energy_sum * 100) / 100;
        last_tick_ms = now;
    }
}

namespace entities
{
    void init(void)
    {
        wsclient_on_message(on_message);
    }

    void update(void)
    {
        tick();

        if (!isServerConnected)
        {
            declared = false;
            return;
        }
        if (!declared)
        {
            declared = true;
            last_alive_ms = millis();
            send_entities(true);
        }
        send_entities(false);

        if (millis() - last_alive_ms >= ALIVE_MS)
        {
            JsonDocument doc;
            doc["type"] = "pub_alive";
            send(doc);
            last_alive_ms = millis();
        }
    }
}
