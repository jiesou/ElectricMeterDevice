#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>

// 面板 64x32 驱动 + U8g2 字库渲染，画什么见 hal/matrix.h
namespace rgb_matrix
{
    // 帧格式固定，屏幕侧 STM32 按此解析，不能改行列顺序
    constexpr int16_t WIDTH = 64;
    constexpr int16_t HEIGHT = 32;

    // 帧格式不变，竖版靠坐标旋转：1 或 3 是竖版（32 宽 64 高），两者相差 180°
    constexpr uint8_t ROTATION = 1;

    constexpr uint16_t BLACK = 0x0000;
    constexpr uint16_t WHITE = 0xFFFF;
    constexpr uint16_t GRAY = 0x8410;
    constexpr uint16_t RED = 0xF800;
    constexpr uint16_t YELLOW = 0xFFE0;
    constexpr uint16_t GREEN = 0x07E0;
    constexpr uint16_t CYAN = 0x07FF;
    constexpr uint16_t BLUE = 0x001F;
    constexpr uint16_t FUCHSIA = 0xF81F;

    // 继承 Adafruit_GFX，drawPixel 即物理写入帧缓冲
    class Canvas : public Adafruit_GFX
    {
    public:
        Canvas(void) : Adafruit_GFX(rgb_matrix::WIDTH, rgb_matrix::HEIGHT) {}
        void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    };

    extern Canvas canvas;

    void init(void);

    // 每次发一整帧，调用频率决定刷新率
    void scanDisplay(void);

    // 字体数组直接传进来：ASCII 用 u8g2_font_4x6_tf，中文用 u8g2_font_wqy12_t_gb2312
    void setFont(const uint8_t *u8g2_font);
    void setColor(uint16_t color);

    // 返回画完这一串的宽度
    int16_t drawUTF8(int16_t x, int16_t y, const char *str);
    void drawUTF8Centered(int16_t y, const char *str);
}
