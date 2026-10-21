#pragma once
#include <Adafruit_GFX.h>

// 面板 64x32 驱动 + 12px 点阵字，画什么见 hal/matrix.h
namespace rgb_matrix
{
    // 帧格式固定，屏幕侧按此解析，不能改行列顺序
    constexpr int16_t WIDTH = 64;
    constexpr int16_t HEIGHT = 32;
    constexpr uint8_t ROTATION = 1;
    constexpr size_t BUFFER_SIZE = rgb_matrix::WIDTH * rgb_matrix::HEIGHT * sizeof(uint16_t);

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
    void init(void);

    void scanDisplay(void);

    int16_t textWidth(const char *text);
    int16_t drawText(int16_t x, int16_t y, const char *text, uint16_t color);
    void drawTextCentered(int16_t y, const char *text, uint16_t color);
}
