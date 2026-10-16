#pragma once
#include "hal/rgb_matrix.h"
#include "hal/ztw_ddsu666.h"

// 屏幕上画什么，读数从 ztw_ddsu666 拿
namespace matrix
{
    void init(void);
    void update(void);

    // 格式化电表读数。一行最多 7 个 4px 字符，多了会顶到边框上
    inline void formatMeter(uint8_t addr, const ztw_ddsu666::Data &meter, char *power, char *energy)
    {
        if (!meter.online)
        {
            snprintf(power, 16, "%u:--W", addr);
            snprintf(energy, 16, "--kWh");
            return;
        }

        if (meter.power_w < 100)
            snprintf(power, 16, "%u:%.1fW", addr, meter.power_w);
        else
            snprintf(power, 16, "%u:%.0fW", addr, meter.power_w);

        if (meter.energy_kwh < 100)
            snprintf(energy, 16, "%.1fkWh", meter.energy_kwh);
        else
            snprintf(energy, 16, "%.0fkWh", meter.energy_kwh);
    }

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
}
