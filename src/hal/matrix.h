#pragma once
#include <stdio.h>
#include "entities.h"
#include "hal/matrix_gfx.h"
#include "hal/ztw_ddsu666.h"
#include "wsclient.h"

// 屏上画什么：上面房间号，中间有人/无人，底部两条 1px 状态条
namespace matrix
{
    // 沿边框走一圈，pos 从左下角起逆时针绕回
    inline void borderPixel(int pos, uint16_t color)
    {
        const int16_t w = canvas.width();
        const int16_t h = canvas.height();
        const int perimeter = 2 * (w + h);
        pos %= perimeter;

        if (pos < h)
            canvas.drawPixel(0, h - 1 - pos, color);
        else if (pos < h + w)
            canvas.drawPixel(pos - h, 0, color);
        else if (pos < 2 * h + w)
            canvas.drawPixel(w - 1, pos - h - w, color);
        else
            canvas.drawPixel(perimeter - 1 - pos, h - 1, color);
    }

    // 彩虹旋转：每 4 像素一段，6 色循环
    inline void drawRainbow(void)
    {
        constexpr uint16_t colors[] = {RED, YELLOW, GREEN, CYAN, BLUE, FUCHSIA};
        const int perimeter = 2 * (canvas.width() + canvas.height());
        const uint8_t anim = millis() / 200;
        for (int pos = 0; pos < perimeter; pos++)
            borderPixel(pos, colors[(pos / 4 + anim) % 6]);
    }

    // 一帧的内容：房间号、有人/无人、三条状态条
    inline void drawContent(void)
    {
        // 人数一变：先让有人/无人停 HOLD_MS，再把新人数闪一下，然后回到常态
        constexpr uint32_t HOLD_MS = 1500;
        constexpr uint32_t FLASH_MS = 2000;
        static int lastCount = 0;
        static uint32_t changedAt = 0;
        static bool changed = false;

        const int count = entities::peopleCount;
        if (count != lastCount)
        {
            lastCount = count;
            changedAt = millis();
            changed = true;
        }
        const uint32_t since = millis() - changedAt;
        const bool flashing = changed && since >= HOLD_MS && since < HOLD_MS + FLASH_MS;

        const bool people = count > 0;
        const uint16_t peopleColor = people ? RED : GREEN;

        textCentered(4, "301", WHITE);

        if (flashing)
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d人", count);
            textCentered(28, buf, peopleColor);
        }
        else
        {
            textCentered(28, people ? "有人" : "无人", peopleColor);
        }

        // 电能表 RS485 连接状态：三段紧挨着，合成一整条；绿=在线，红=掉线
        constexpr int16_t SEGMENT_WIDTH = 11;
        for (size_t i = 0; i < ztw_ddsu666::METER_COUNT; i++)
            canvas.drawFastHLine(i * SEGMENT_WIDTH, WIDTH - 3, SEGMENT_WIDTH,
                                 ztw_ddsu666::meters[i].online ? GREEN : RED);

        // WebSocket 服务器连接状态
        canvas.drawFastHLine(0, WIDTH - 2, ztw_ddsu666::METER_COUNT * SEGMENT_WIDTH,
                             wsclient::isServerConnected ? GREEN : RED);
    }

    void init(void);
    void update(void);
}
