#pragma once
#include "entities.h"
#include "hal/rgb_matrix.h"
#include "hal/ztw_ddsu666.h"
#include "wsclient.h"

// 屏上画什么：上面房间号，中间有人/无人，底部两条 1px 状态条
namespace matrix
{
    // 沿边框走一圈，pos 从左下角起逆时针绕回
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

    // 一帧的内容：房间号、有人/无人、三条状态条
    inline void drawContent(void)
    {
        constexpr int16_t NAME_Y = 6;    // 房间号 12px 一行
        constexpr int16_t PEOPLE_Y = 28; // 有人/无人
        constexpr int16_t METER_Y = 54;  // 每只电表占一段
        constexpr int16_t WS_Y = 60;     // 最底下一条是 WebSocket
        constexpr int16_t SEGMENT_WIDTH = 10;

        rgb_matrix::drawTextCentered(NAME_Y, "301", rgb_matrix::WHITE);

        const bool people = entities::peopleCount > 0;
        rgb_matrix::drawTextCentered(PEOPLE_Y, people ? "有人" : "无人", people ? rgb_matrix::RED : rgb_matrix::GREEN);

        // 三段紧挨着，合成一整条；绿=在线，红=掉线
        for (size_t i = 0; i < ztw_ddsu666::METER_COUNT; i++)
            rgb_matrix::canvas.drawFastHLine(1 + i * SEGMENT_WIDTH, METER_Y, SEGMENT_WIDTH,
                                             ztw_ddsu666::meters[i].online ? rgb_matrix::GREEN : rgb_matrix::RED);

        rgb_matrix::canvas.drawFastHLine(1, WS_Y, ztw_ddsu666::METER_COUNT * SEGMENT_WIDTH,
                                         wsclient::isServerConnected ? rgb_matrix::GREEN : rgb_matrix::RED);
    }

    void init(void);
    void update(void);
}
