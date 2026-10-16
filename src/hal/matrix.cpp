#include "hal/matrix.h"
#include <U8g2lib.h>
#include "entities.h"
#include "hal/rgb_matrix.h"
#include "hal/ztw_ddsu666.h"

namespace matrix
{
    constexpr int16_t TOP = 21; // 顶上一块留给人数
    constexpr int16_t ROW = 14; // 下面三块电表均分

    void init(void)
    {
        rgb_matrix::init();
    }

    void update(void)
    {
        static uint32_t lastUpdate = 0;
        uint32_t now = millis();
        if (now - lastUpdate < 200)
            return;
        lastUpdate = now;

        rgb_matrix::canvas.fillScreen(rgb_matrix::BLACK);
        drawRainbow();

        // 房间人数：服务器推下来的净数，一个大数字顶在最上面
        char people[16];
        snprintf(people, sizeof(people), "%d", entities::peopleCount);
        rgb_matrix::setFont(u8g2_font_logisoso22_tn);
        rgb_matrix::setColor(rgb_matrix::CYAN);
        rgb_matrix::drawUTF8Centered(20, people);

        rgb_matrix::setFont(u8g2_font_4x6_tf);
        for (size_t i = 0; i < ztw_ddsu666::METER_COUNT; i++)
        {
            const int16_t y = TOP + i * ROW;
            char power[16], energy[16];
            formatMeter(i + 1, ztw_ddsu666::meters[i], power, energy);

            rgb_matrix::setColor(ztw_ddsu666::meters[i].online ? rgb_matrix::GREEN : rgb_matrix::RED);
            rgb_matrix::drawUTF8(2, y + 6, power);

            rgb_matrix::setColor(rgb_matrix::WHITE);
            rgb_matrix::drawUTF8(2, y + 13, energy);
        }

        rgb_matrix::scanDisplay();
    }
}
