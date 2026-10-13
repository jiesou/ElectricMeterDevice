#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>

namespace matrix
{
    constexpr int16_t WIDTH = 64;
    constexpr int16_t HEIGHT = 32;

    constexpr uint16_t WHITE = 0xFFFF;
    constexpr uint16_t RED = 0xF800;
    constexpr uint16_t YELLOW = 0xFFE0;
    constexpr uint16_t GREEN = 0x07E0;
    constexpr uint16_t CYAN = 0x07FF;
    constexpr uint16_t BLUE = 0x001F;
    constexpr uint16_t FUCHSIA = 0xF81F;

    void init(void);
    void update(void);
    void clear(void);
    void scanDisplay(void);

    // Adafruit_GFX 绘图接口，坐标按 rotation 变换后写入帧缓冲
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    // U8g2 字体渲染，支持中文
    void setFont(const uint8_t *u8g2_font);
    void setColor(uint16_t color);
    int16_t drawUTF8(int16_t x, int16_t y, const char *str);
    void drawUTF8Centered(int16_t y, const char *str);

    // ---- 边框动画，anim_state 每帧递增即为旋转 ----
    void drawRainbow(uint8_t anim_state);
    void drawRed(uint8_t anim_state);
    void drawGreen(uint8_t anim_state);
    void drawBlue(uint8_t anim_state);
}
