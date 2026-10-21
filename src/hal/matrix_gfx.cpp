#include "hal/matrix_gfx.h"
#include <Arduino.h>
#include <SPI.h>
#include "font_data.h"
#include "pins.h"

// 32x64 竖版 1/16 扫描 HUB75 驱动，屏幕侧自带扫描逻辑
namespace
{
    constexpr uint32_t SPI_SPEED = 700000;

    SPIClass spi(HSPI);
    SPISettings spi_settings(SPI_SPEED, MSBFIRST, SPI_MODE0);

    const font::Glyph *find(uint16_t codepoint)
    {
        for (size_t i = 0; i < font::COUNT; i++)
            if (font::GLYPHS[i].codepoint == codepoint)
                return &font::GLYPHS[i];
        return nullptr;
    }

    uint16_t utf8Next(const char *&text)
    {
        uint8_t c = *text++;
        if (c < 0x80)
            return c;

        uint16_t codepoint = 0;
        if ((c & 0xE0) == 0xC0)
        {
            codepoint = (c & 0x1F) << 6;
            codepoint |= (*text++ & 0x3F);
        }
        else if ((c & 0xF0) == 0xE0)
        {
            codepoint = (c & 0x0F) << 12;
            codepoint |= (*text++ & 0x3F) << 6;
            codepoint |= (*text++ & 0x3F);
        }
        return codepoint;
    }
}

namespace matrix
{
    GFXcanvas16 canvas(WIDTH, HEIGHT);

    void begin(void)
    {
        spi.begin(PIN_MATRIX_SCK, -1, PIN_MATRIX_MOSI, PIN_MATRIX_CS);
        pinMode(PIN_MATRIX_CS, OUTPUT);
        digitalWrite(PIN_MATRIX_CS, HIGH);
        canvas.setRotation(ROTATION);
        canvas.fillScreen(BLACK);
    }

    void scanDisplay(void)
    {
        spi.beginTransaction(spi_settings);
        digitalWrite(PIN_MATRIX_CS, LOW);
        delayMicroseconds(5);

        // 从机按大端收 uint16_t，ESP32 是小端，发之前整帧换一遍字节序
        canvas.byteSwap();
        spi.transferBytes((uint8_t *)canvas.getBuffer(), nullptr, BUFFER_SIZE);
        canvas.byteSwap();

        delayMicroseconds(2);
        digitalWrite(PIN_MATRIX_CS, HIGH);
        spi.endTransaction();
    }

    int16_t textWidth(const char *txt)
    {
        int16_t width = 0;
        while (*txt)
        {
            const font::Glyph *glyph = find(utf8Next(txt));
            if (glyph)
                width += glyph->xAdvance;
        }
        return width;
    }

    int16_t text(int16_t x, int16_t y, const char *txt, uint16_t color)
    {
        int16_t start = x;
        while (*txt)
        {
            const font::Glyph *glyph = find(utf8Next(txt));
            if (!glyph)
                continue;

            const uint8_t stride = (glyph->width + 7) / 8;
            const uint8_t *bits = font::BITMAP + glyph->bitmapOffset;
            for (uint8_t row = 0; row < glyph->height; row++)
                for (uint8_t col = 0; col < glyph->width; col++)
                    if (bits[row * stride + col / 8] & (0x80 >> (col % 8)))
                        canvas.drawPixel(x + glyph->xOffset + col, y + glyph->yOffset + row, color);

            x += glyph->xAdvance;
        }
        return x - start;
    }

    void textCentered(int16_t y, const char *txt, uint16_t color)
    {
        int16_t x = (canvas.width() - textWidth(txt)) / 2;
        text(x < 0 ? 0 : x, y, txt, color);
    }
}
