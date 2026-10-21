#pragma once
#include <Adafruit_GFX.h>

// 面板驱动：怎么把像素送上屏。画什么见 hal/matrix.h
namespace matrix
{
    // 帧格式固定，屏幕侧按此解析，不能改行列顺序
    constexpr int16_t WIDTH = 64;
    constexpr int16_t HEIGHT = 32;
    constexpr uint8_t ROTATION = 1;
    constexpr size_t BUFFER_SIZE = WIDTH * HEIGHT * sizeof(uint16_t);

    constexpr uint16_t BLACK = 0x0000;
    constexpr uint16_t WHITE = 0xFFFF;
    constexpr uint16_t GRAY = 0x8410;
    constexpr uint16_t RED = 0xF800;
    constexpr uint16_t YELLOW = 0xFFE0;
    constexpr uint16_t GREEN = 0x07E0;
    constexpr uint16_t CYAN = 0x07FF;
    constexpr uint16_t BLUE = 0x001F;
    constexpr uint16_t FUCHSIA = 0xF81F;

    extern GFXcanvas16 canvas;

    // 开面板，整屏初始化见 init()
    void begin(void);

    void scanDisplay(void);

    int16_t textWidth(const char *text);
    int16_t text(int16_t x, int16_t y, const char *text, uint16_t color);
    void textCentered(int16_t y, const char *text, uint16_t color);
}
