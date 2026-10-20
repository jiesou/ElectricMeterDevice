#pragma once
#include <U8g2lib.h>
#include "entities.h"
#include "hal/relay.h"
#include "hal/rgb_matrix.h"
#include "hal/ztw_ddsu666.h"

// 屏幕上画什么，读数从 ztw_ddsu666 拿
namespace matrix
{
    void init(void);
    void update(void);

    // ---- 彩色边框动画，相位按时间自己走，跟 update 的 200ms 一拍 ----

    // 沿边框走一圈，pos 从左下角起逆时针绕回。画布是竖版，宽高跟帧缓冲反着
    inline void borderPixel(int pos, uint16_t color)
    {
        const int16_t w = rgb_matrix::canvas.width();
        const int16_t h = rgb_matrix::canvas.height();
        const int perimeter = 2 * (w + h);
        pos %= perimeter;

        if (pos < h)
            rgb_matrix::canvas.drawPixel(0, h - 1 - pos, color);
        else if (pos < h + w)
            rgb_matrix::canvas.drawPixel(pos - h, 0, color);
        else if (pos < 2 * h + w)
            rgb_matrix::canvas.drawPixel(w - 1, pos - h - w, color);
        else
            rgb_matrix::canvas.drawPixel(perimeter - 1 - pos, h - 1, color);
    }

    // 彩虹旋转：每 4 像素一段，6 色循环
    inline void drawRainbow(void)
    {
        constexpr uint16_t colors[] = {rgb_matrix::RED, rgb_matrix::YELLOW, rgb_matrix::GREEN, rgb_matrix::CYAN, rgb_matrix::BLUE, rgb_matrix::FUCHSIA};
        const int perimeter = 2 * (rgb_matrix::canvas.width() + rgb_matrix::canvas.height());
        const uint8_t anim = millis() / 200;
        for (int pos = 0; pos < perimeter; pos++)
            borderPixel(pos, colors[(pos / 4 + anim) % 6]);
    }

    // 一帧的内容：顶上一行人数，下面三行电表。一行最多 7 个 4px 字符，多了会顶到边框上
    // 表掉线也照画上一次的读数，屏幕上不体现掉线
    inline void drawContent(void)
    {
        char text[16];
        snprintf(text, sizeof(text), "%d", entities::peopleCount);
        rgb_matrix::drawUTF8Centered(20, text, rgb_matrix::CYAN);

        for (size_t i = 0; i < ztw_ddsu666::METER_COUNT; i++)
        {
            const ztw_ddsu666::Data &meter = ztw_ddsu666::meters[i];
            const Entity *relay = entities::findById(relay::ENTITY_RELAY_MAP[i].first);
            const bool on = relay && relay->state.value_or(false);
            const int16_t y = 21 + i * 14;

            // 行顶那条 1px 横线就是这一路的闸：绿=合，灰=断
            rgb_matrix::canvas.drawFastHLine(2, y, 28, on ? rgb_matrix::GREEN : rgb_matrix::GRAY);

            snprintf(text, sizeof(text), "%u:%.0fW", (unsigned)(i + 1), meter.power_w);
            rgb_matrix::drawUTF8(2, y + 6, text, rgb_matrix::GREEN);

            snprintf(text, sizeof(text), "%.1fkWh", meter.energy_kwh);
            rgb_matrix::drawUTF8(2, y + 13, text, rgb_matrix::WHITE);
        }
    }
}
