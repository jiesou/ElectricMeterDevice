#pragma once
#include "hal/rgb_matrix.h"
#include "hal/ztw_ddsu666.h"

// 屏幕上画什么，读数从 ztw_ddsu666 拿
namespace matrix
{
    void init(void);
    void update(void);

    // 格式化电表读数
    inline void formatMeter(uint8_t addr, const ztw_ddsu666::Data &meter, char *power, char *energy)
    {
        if (!meter.online)
        {
            snprintf(power, 16, "%u:--W", addr);
            snprintf(energy, 16, "--kWh");
            return;
        }

        if (meter.power_w < 1000)
            snprintf(power, 16, "%u:%.1fW", addr, meter.power_w);
        else
            snprintf(power, 16, "%u:%.0fW", addr, meter.power_w);

        if (meter.energy_kwh < 1000)
            snprintf(energy, 16, "%.1fkWh", meter.energy_kwh);
        else
            snprintf(energy, 16, "%.0fkWh", meter.energy_kwh);
    }

    // ---- 边框动画，相位按时间自己走，跟 update 的 200ms 一拍 ----
    constexpr int PERIMETER = 2 * (rgb_matrix::WIDTH + rgb_matrix::HEIGHT);

    // 沿边框走一圈，pos 从下边左侧起绕回
    inline void borderPixel(int pos, uint16_t color)
    {
        pos %= PERIMETER;
        if (pos < rgb_matrix::HEIGHT)
            rgb_matrix::canvas.drawPixel(0, rgb_matrix::HEIGHT - 1 - pos, color);
        else if (pos < rgb_matrix::HEIGHT + rgb_matrix::WIDTH)
            rgb_matrix::canvas.drawPixel(pos - rgb_matrix::HEIGHT, 0, color);
        else if (pos < 2 * rgb_matrix::HEIGHT + rgb_matrix::WIDTH)
            rgb_matrix::canvas.drawPixel(rgb_matrix::WIDTH - 1, pos - rgb_matrix::HEIGHT - rgb_matrix::WIDTH, color);
        else
            rgb_matrix::canvas.drawPixel(PERIMETER - 1 - pos, rgb_matrix::HEIGHT - 1, color);
    }

    // 虚线流动：4 像素亮 4 像素灭
    inline void drawDashed(uint16_t color)
    {
        constexpr int dash = 4, period = 8;
        uint8_t anim = millis() / 200;
        for (int pos = 0; pos < PERIMETER; pos++)
            if ((pos + anim) % period < dash)
                borderPixel(pos, color);
    }

    // 彩虹旋转：每 4 像素一段，6 色循环
    inline void drawRainbow(void)
    {
        constexpr uint16_t colors[] = {rgb_matrix::RED, rgb_matrix::YELLOW, rgb_matrix::GREEN, rgb_matrix::CYAN, rgb_matrix::BLUE, rgb_matrix::FUCHSIA};
        uint8_t anim = millis() / 200;
        for (int pos = 0; pos < PERIMETER; pos++)
            borderPixel(pos, colors[(pos / 4 + anim) % 6]);
    }

    inline void drawRed(void) { drawDashed(rgb_matrix::RED); }
    inline void drawGreen(void) { drawDashed(rgb_matrix::GREEN); }
    inline void drawBlue(void) { drawDashed(rgb_matrix::BLUE); }
}
