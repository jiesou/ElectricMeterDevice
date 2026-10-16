#include "entities.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <deque>
#include <vector>
#include "wsclient.h"
#include "hal/time_ntp.h"
#include "hal/ztw_ddsu666.h"

namespace entities
{
    std::vector<Entity> data = {
        {"light", "灯", "switch", false, 0, 0, 0},
        {"light-meter", "灯功率", "meter", false, 0, 0, 0},
        {"kettle", "热水壶", "switch", false, 0, 0, 0},
        {"kettle-meter", "热水壶功率", "meter", false, 0, 0, 0},
        {"dryer", "吹风机", "switch", false, 0, 0, 0},
        {"dryer-meter", "吹风机功率", "meter", false, 0, 0, 0},
    };

    std::deque<Entity> queue;
}

namespace
{
    String wsMessageStr;

    void on_message(const String &message)
    {
        JsonDocument doc;
        if (deserializeJson(doc, message))
            return;
        if (strcmp(doc["type"] | "", "pub_action_switch") != 0)
            return;

        for (Entity &e : entities::data)
        {
            if (strcmp(doc["id"] | "", e.id) != 0)
                continue;

            e.state = doc["state"] | false;

            JsonDocument ack;
            ack["type"] = "ack_action_switch";
            ack["id"] = e.id;
            ack["state"] = e.state;

            serializeJson(ack, wsMessageStr);
            wsclient::send_message(wsMessageStr);
        }
    }
}

namespace entities
{
    void init(void)
    {
        wsclient::on_message(on_message);

        // declare entities
        JsonDocument doc;
        doc["type"] = "pub_entities";
        JsonArray list = doc["entities"].to<JsonArray>();
        for (const Entity &e : data)
        {
            JsonObject o = list.add<JsonObject>();
            o["id"] = e.id;
            o["name"] = e.name;
            o["type"] = e.type;
        }

        serializeJson(doc, wsMessageStr);
        wsclient::send_message(wsMessageStr);

        ztw_ddsu666::on_data([](size_t i)
                             {
                    const ztw_ddsu666::Data &data = ztw_ddsu666::meters[i];
        const Entity &profile = entities::data[i * 2 + 1];

        Entity read = {};
        read.id = profile.id;
        read.powerW = data.power_w;
        read.energyKwh = data.energy_kwh;
        read.ts = time(nullptr);
        entities::queue.push_back(read); });
    }

    void update(void)
    {
        static uint32_t lastUpdate = 0;
        uint32_t now = millis();
        if (now - lastUpdate < 1000)
            return;
        lastUpdate = now;

        if (!queue.empty())
        {
            JsonDocument doc;
            doc["type"] = "pub_entities";
            doc["ts"] = time_ntp::now();
            JsonArray list = doc["entities"].to<JsonArray>();
            for (const Entity &e : queue)
            {
                JsonObject o = list.add<JsonObject>();
                o["id"] = e.id;
                o["ts"] = e.ts;
                if (e.type[0] == 's') // "switch"
                    o["state"] = e.state;
                else
                {
                    o["powerW"] = e.powerW;
                    o["energyKwh"] = e.energyKwh;
                }
            }
            queue.clear();

            serializeJson(doc, wsMessageStr);
            wsclient::send_message(wsMessageStr);
        }
    }
}
