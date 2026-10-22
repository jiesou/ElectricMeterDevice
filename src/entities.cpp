#include "entities.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <deque>
#include <vector>
#include "wsclient.h"
#include "hal/relay.h"
#include "hal/server_time.h"
#include "hal/ztw_ddsu666.h"
#include "hal/buzzer_player.h"

namespace entities
{
    std::vector<Entity> data = {
        {"light", "switch", "灯", true},
        {"light-meter", "meter", "灯功率"},
        {"kettle", "switch", "热水壶", true},
        {"kettle-meter", "meter", "热水壶功率"},
        {"dryer", "switch", "吹风机", true},
        {"dryer-meter", "meter", "吹风机功率"},
    };

    std::deque<Entity> queue;
    int peopleCount = 0;
    char roomName[24] = "";
}

namespace
{
    String wsMessageStr;

    // 待发队列的兜底上限：正常每秒清一次，攒不到这么多。真攒满了就丢新读数，
    // 不 pop_front——队列开头是 6 条实体申明，丢了服务器就再也收不到开关声明
    constexpr size_t QUEUE_MAX = 32;

    void on_message(const String &message)
    {
        JsonDocument doc;
        if (deserializeJson(doc, message))
            return;

        // 心跳回包带着服务器的毫秒，本机时间就跟着它走
        if (strcmp(doc["type"] | "", "ack_alive") == 0)
        {
            server_time::sync(doc["ts"] | 0ULL);
            return;
        }

        // 房名由服务器推下来：网页上改名，屏幕上那行字就跟着变
        if (strcmp(doc["type"] | "", "pub_name") == 0)
        {
            snprintf(entities::roomName, sizeof(entities::roomName), "%s", (const char *)(doc["name"] | ""));
            return;
        }

        // 服务器把房间人数推下来：大屏算的净数照收，不自己拿进出相减
        if (strcmp(doc["type"] | "", "pub_entities") == 0)
        {
            for (JsonObject e : doc["entities"].as<JsonArray>())
                if (strcmp(e["type"] | "", "people-counter") == 0) {
                    const int newNetCount = (int)(e["netCount"] | 0);
                    if (newNetCount > entities::peopleCount) {
                        buzzer_player::success();
                    } else if (newNetCount < entities::peopleCount) {
                        buzzer_player::fail();
                    }
                    entities::peopleCount = e["netCount"] | 0;
                }
            return;
        }

        if (strcmp(doc["type"] | "", "pub_action_switch") != 0)
            return;

        Entity *e = entities::findById(doc["id"] | "");
        if (!e || strcmp(e->type, "switch") != 0)
            return; // 只有开关能被操作

        e->state = doc["state"] | false;
        relay::set(e->id, *e->state);
        if (*e->state)
            buzzer_player::success();
        else
            buzzer_player::fail();

        JsonDocument ack;
        ack["type"] = "ack_action_switch";
        ack["id"] = e->id;
        ack["state"] = *e->state;
        const uint64_t ts = server_time::now();
        if (ts)
            ack["ts"] = ts;

        serializeJson(ack, wsMessageStr);
        wsclient::send_message(wsMessageStr);
    }
}

namespace entities
{
    void init(void)
    {
        wsclient::on_message(on_message);

        // 声明初始化数据
        queue.insert(queue.end(), data.begin(), data.end());

        // 读数往这一步对应那只电表实体上填
        ztw_ddsu666::on_data([](size_t i)
                             {
        // 没连上就不记读数：断网这几分钟里电表每秒都在出新读数，重连后发的本来就是最新的。
        // 之前照单全收，队列越攒越长越吃堆，堆不够时 push_back 分配失败直接 abort 重启
        if (!wsclient::isServerConnected || entities::queue.size() >= QUEUE_MAX)
            return;

        Entity *profile = entities::findById(ztw_ddsu666::ENTITY_METER_ADDR_MAP[i]);
        if (!profile)
            return;
        const ztw_ddsu666::Data &data = ztw_ddsu666::meters[i];

        Entity read{}; // 只填读数真有的字段，id/type 从申报那条拿
        read.id = profile->id;
        read.type = profile->type;
        read.powerW = data.power_w;
        read.energyKwh = data.energy_kwh;
        const uint64_t ts = server_time::now();
        if (ts)
            read.ts = ts;
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
        const uint64_t ts = server_time::now();
        if (ts)
            doc["ts"] = ts;
        JsonArray list = doc["entities"].to<JsonArray>();
        for (const Entity &e : queue)
        {
            // 把队列里的东西合并到同一个pub_entities请求
            JsonObject o = list.add<JsonObject>();
            o["id"] = e.id;
            o["type"] = e.type;
            if (e.name)
                o["name"] = *e.name;
            if (e.state)
                o["state"] = *e.state;
            if (e.powerW)
                o["powerW"] = *e.powerW;
            if (e.energyKwh)
                o["energyKwh"] = *e.energyKwh;
            if (e.ts)
                o["ts"] = *e.ts;
        }
        queue.clear();

        serializeJson(doc, wsMessageStr);
        Serial.printf("[entites] ms=%u push %u entities, %u bytes\n", millis(), (unsigned)doc["entities"].size(), wsMessageStr.length());
        wsclient::send_message(wsMessageStr);
    }
}
