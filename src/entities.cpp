#include "entities.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <deque>
#include <vector>
#include "wsclient.h"
#include "hal/relay.h"
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
    int peopleCount = 0;
}

namespace
{
    String wsMessageStr;

    void on_message(const String &message)
    {
        JsonDocument doc;
        if (deserializeJson(doc, message))
            return;

        // 服务器把房间人数推下来：大屏算的净数照收，不自己拿进出相减
        if (strcmp(doc["type"] | "", "pub_entities") == 0)
        {
            for (JsonObject e : doc["entities"].as<JsonArray>())
                if (strcmp(e["type"] | "", "people-counter") == 0)
                    entities::peopleCount = e["netCount"] | 0;
            return;
        }

        if (strcmp(doc["type"] | "", "pub_action_switch") != 0)
            return;

        // 实体表是 开关/电表 交替排列，第 i 只开关就是第 i 路继电器
        for (size_t i = 0; i < entities::data.size(); i++)
        {
            Entity &e = entities::data[i];
            if (strcmp(doc["id"] | "", e.id) != 0)
                continue;
            if (strcmp(e.type, "switch") != 0)
                return; // 只有开关能被操作

            e.state = doc["state"] | false;
            relay::set(i / 2, e.state);

            JsonDocument ack;
            ack["type"] = "ack_action_switch";
            ack["id"] = e.id;
            ack["state"] = e.state;
            ack["ts"] = time_ntp::now();

            serializeJson(ack, wsMessageStr);
            wsclient::send_message(wsMessageStr);
            return;
        }
    }
}

namespace entities
{
    void init(void)
    {
        wsclient::on_message(on_message);

        // 声明初始化数据
        queue.insert(queue.end(), data.begin(), data.end());

        ztw_ddsu666::on_data([](size_t i)
                             {
        const ztw_ddsu666::Data &data = ztw_ddsu666::meters[i];
        const Entity &profile = entities::data[i * 2 + 1];

        Entity read = profile; // 读数复用申报那条的 id/name/type
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

        if (!wsclient::isServerConnected || queue.empty())
            return; // 没连上就留着，连上了再发

        JsonDocument doc;
        doc["type"] = "pub_entities";
        doc["ts"] = time_ntp::now();
        JsonArray list = doc["entities"].to<JsonArray>();
        for (const Entity &e : queue)
        {
            // 把队列里的东西合并到同一个pub_entities请求
            JsonObject o = list.add<JsonObject>();
            o["id"] = e.id;
            o["name"] = e.name;
            o["type"] = e.type;
            if (e.ts) // 申报那条还没读数，没有 ts
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
