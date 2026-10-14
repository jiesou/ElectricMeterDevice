#include "hal/matrix.h"
#include <SPI.h>
#include <U8g2lib.h>
#include "hal/ztw_ddsu666.h"
#include "pins.h"

// 32x64 竖版 1/16 扫描 HUB75 驱动，屏幕侧自带扫描逻辑
namespace
{
    constexpr uint32_t SPI_SPEED = 700000;

    uint16_t framebuffer[matrix::HEIGHT][matrix::WIDTH];
    SPIClass spi(HSPI);
    SPISettings spi_settings(SPI_SPEED, MSBFIRST, SPI_MODE0);

    uint16_t fg_color = matrix::WHITE;

    // ---- U8g2 字体解码，参考 U8g2_for_Adafruit_GFX ----
    struct FontInfo
    {
        uint8_t glyph_cnt;
        uint8_t bbx_mode;
        uint8_t bits_per_0;
        uint8_t bits_per_1;
        uint8_t bits_per_char_width;
        uint8_t bits_per_char_height;
        uint8_t bits_per_char_x;
        uint8_t bits_per_char_y;
        uint8_t bits_per_delta_x;
        int8_t max_char_width;
        int8_t max_char_height;
        int8_t x_offset;
        int8_t y_offset;
        int8_t ascent_A;
        int8_t descent_g;
        int8_t ascent_para;
        int8_t descent_para;
        uint16_t start_pos_upper_A;
        uint16_t start_pos_lower_a;
        uint16_t start_pos_unicode;
    };

    const uint8_t *font = nullptr;
    FontInfo font_info;

    uint8_t getFontByte(uint8_t offset)
    {
        return u8x8_pgm_read(font + offset);
    }

    uint16_t getFontWord(uint8_t offset)
    {
        return ((uint16_t)getFontByte(offset) << 8) | getFontByte(offset + 1);
    }

    // 字体头部 23 字节，字段顺序固定
    void readFontInfo(void)
    {
        font_info.glyph_cnt = getFontByte(0);
        font_info.bbx_mode = getFontByte(1);
        font_info.bits_per_0 = getFontByte(2);
        font_info.bits_per_1 = getFontByte(3);
        font_info.bits_per_char_width = getFontByte(4);
        font_info.bits_per_char_height = getFontByte(5);
        font_info.bits_per_char_x = getFontByte(6);
        font_info.bits_per_char_y = getFontByte(7);
        font_info.bits_per_delta_x = getFontByte(8);
        font_info.max_char_width = getFontByte(9);
        font_info.max_char_height = getFontByte(10);
        font_info.x_offset = getFontByte(11);
        font_info.y_offset = getFontByte(12);
        font_info.ascent_A = getFontByte(13);
        font_info.descent_g = getFontByte(14);
        font_info.ascent_para = getFontByte(15);
        font_info.descent_para = getFontByte(16);
        font_info.start_pos_upper_A = getFontWord(17);
        font_info.start_pos_lower_a = getFontWord(19);
        font_info.start_pos_unicode = getFontWord(21);
    }

    // 字形数据按位紧排，需要跨字节取任意位宽
    struct Decode
    {
        const uint8_t *ptr;
        uint8_t bit_pos;
        int16_t target_x;
        int16_t target_y;
        int8_t x;
        int8_t y;
        int8_t glyph_width;
        int8_t glyph_height;
    };

    Decode decode;

    uint8_t getUnsignedBits(uint8_t cnt)
    {
        uint8_t val = u8x8_pgm_read(decode.ptr);
        val >>= decode.bit_pos;
        uint8_t bit_pos_plus_cnt = decode.bit_pos + cnt;

        if (bit_pos_plus_cnt >= 8)
        {
            uint8_t s = 8 - decode.bit_pos;
            decode.ptr++;
            val |= u8x8_pgm_read(decode.ptr) << s;
            bit_pos_plus_cnt -= 8;
        }
        val &= (1U << cnt) - 1;
        decode.bit_pos = bit_pos_plus_cnt;
        return val;
    }

    int8_t getSignedBits(uint8_t cnt)
    {
        int8_t v = (int8_t)getUnsignedBits(cnt);
        int8_t d = 1;
        cnt--;
        d <<= cnt;
        v -= d;
        return v;
    }

    // 游程从左到右、到头换行，每段都是水平的一段
    void decodeLen(uint8_t len, bool is_foreground)
    {
        uint8_t cnt = len;
        uint8_t lx = decode.x;
        uint8_t ly = decode.y;

        while (true)
        {
            uint8_t rem = decode.glyph_width - lx;
            uint8_t current = (cnt < rem) ? cnt : rem;

            if (current > 0 && is_foreground)
                for (uint8_t i = 0; i < current; i++)
                    matrix::canvas.drawPixel(decode.target_x + lx + i, decode.target_y + ly, fg_color);

            if (cnt < rem)
                break;
            cnt -= rem;
            lx = 0;
            ly++;
        }
        decode.x = lx + cnt;
        decode.y = ly;
    }

    const uint8_t *getGlyphData(uint16_t encoding)
    {
        const uint8_t *glyph = font + 23;

        if (encoding <= 255)
        {
            if (encoding >= 'a')
                glyph += font_info.start_pos_lower_a;
            else if (encoding >= 'A')
                glyph += font_info.start_pos_upper_A;

            while (true)
            {
                if (u8x8_pgm_read(glyph + 1) == 0)
                    break;
                if (u8x8_pgm_read(glyph) == encoding)
                    return glyph + 2;
                glyph += u8x8_pgm_read(glyph + 1);
            }
        }
        else
        {
            glyph += font_info.start_pos_unicode;
            const uint8_t *lookup = glyph;

            // 先在索引表里跳到目标块，再在块内线性查
            uint16_t e;
            do
            {
                glyph += getFontWord((uint8_t)(lookup - font));
                e = getFontWord((uint8_t)(lookup - font + 2));
                lookup += 4;
            } while (e < encoding);

            while (true)
            {
                e = u8x8_pgm_read(glyph);
                e <<= 8;
                e |= u8x8_pgm_read(glyph + 1);
                if (e == 0)
                    break;
                if (e == encoding)
                    return glyph + 3;
                glyph += u8x8_pgm_read(glyph + 2);
            }
        }
        return nullptr;
    }

    int8_t decodeGlyph(const uint8_t *glyph_data)
    {
        decode.ptr = glyph_data;
        decode.bit_pos = 0;

        decode.glyph_width = getUnsignedBits(font_info.bits_per_char_width);
        decode.glyph_height = getUnsignedBits(font_info.bits_per_char_height);

        int8_t x = getSignedBits(font_info.bits_per_char_x);
        int8_t y = getSignedBits(font_info.bits_per_char_y);
        int8_t d = getSignedBits(font_info.bits_per_delta_x);

        int8_t h = decode.glyph_height;

        if (decode.glyph_width > 0)
        {
            decode.target_x += x;
            decode.target_y += -(h + y);
            decode.x = 0;
            decode.y = 0;

            while (true)
            {
                uint8_t a = getUnsignedBits(font_info.bits_per_0);
                uint8_t b = getUnsignedBits(font_info.bits_per_1);
                do
                {
                    decodeLen(a, false);
                    decodeLen(b, true);
                } while (getUnsignedBits(1) != 0);

                if (decode.y >= h)
                    break;
            }
        }
        return d;
    }

    uint16_t utf8Decode(const char *&str)
    {
        uint8_t c = *str++;
        if (c < 0x80)
            return c;

        uint16_t unicode = 0;
        if ((c & 0xE0) == 0xC0)
        {
            unicode = (c & 0x1F) << 6;
            unicode |= (*str++ & 0x3F);
        }
        else if ((c & 0xF0) == 0xE0)
        {
            unicode = (c & 0x0F) << 12;
            unicode |= (*str++ & 0x3F) << 6;
            unicode |= (*str++ & 0x3F);
        }
        return unicode;
    }

    // 只读步进宽度，用于测量排版
    int8_t glyphDelta(uint16_t encoding)
    {
        const uint8_t *glyph = getGlyphData(encoding);
        if (!glyph)
            return 0;
        decode.ptr = glyph;
        decode.bit_pos = 0;
        getUnsignedBits(font_info.bits_per_char_width);
        getUnsignedBits(font_info.bits_per_char_height);
        getSignedBits(font_info.bits_per_char_x);
        getSignedBits(font_info.bits_per_char_y);
        return getSignedBits(font_info.bits_per_delta_x);
    }

    int16_t utf8Width(const char *str)
    {
        int16_t sum = 0;
        while (*str)
        {
            uint16_t encoding = utf8Decode(str);
            if (encoding == 0)
                break;
            sum += glyphDelta(encoding);
        }
        return sum;
    }

    constexpr int PERIMETER = 2 * (matrix::WIDTH + matrix::HEIGHT);

    // 沿边框走一圈，pos 从下边左侧起绕回
    void borderPixel(int pos, uint16_t color)
    {
        pos %= PERIMETER;
        if (pos < matrix::HEIGHT)
            matrix::canvas.drawPixel(0, matrix::HEIGHT - 1 - pos, color);
        else if (pos < matrix::HEIGHT + matrix::WIDTH)
            matrix::canvas.drawPixel(pos - matrix::HEIGHT, 0, color);
        else if (pos < 2 * matrix::HEIGHT + matrix::WIDTH)
            matrix::canvas.drawPixel(matrix::WIDTH - 1, pos - matrix::HEIGHT - matrix::WIDTH, color);
        else
            matrix::canvas.drawPixel(PERIMETER - 1 - pos, matrix::HEIGHT - 1, color);
    }

    // 虚线段，anim_state 递增即为流动
    void dashedBorder(uint16_t color, uint8_t anim_state)
    {
        constexpr int dash = 4, gap = 4, period = dash + gap;
        for (int pos = 0; pos < PERIMETER; pos++)
            if ((pos + anim_state) % period < dash)
                borderPixel(pos, color);
    }
}

namespace matrix
{
    Canvas canvas;

    // 帧格式不变，仍写 64x32 帧缓冲；rotation 在这里做坐标重映射（同 Adafruit GFXcanvas16）
    void Canvas::drawPixel(int16_t x, int16_t y, uint16_t color)
    {
        if (x < 0 || y < 0 || x >= _width || y >= _height)
            return;

        int16_t t;
        switch (rotation)
        {
        case 1:
            t = x;
            x = WIDTH - 1 - y;
            y = t;
            break;
        case 2:
            x = WIDTH - 1 - x;
            y = HEIGHT - 1 - y;
            break;
        case 3:
            t = x;
            x = y;
            y = HEIGHT - 1 - t;
            break;
        }
        framebuffer[y][x] = color;
    }

    void init(void)
    {
        spi.begin(PIN_MATRIX_SCK, -1, PIN_MATRIX_MOSI, PIN_MATRIX_CS);
        pinMode(PIN_MATRIX_CS, OUTPUT);
        digitalWrite(PIN_MATRIX_CS, HIGH);
        canvas.setRotation(ROTATION);
        canvas.fillScreen(BLACK);
    }

    // 每次发一整帧，调用频率决定刷新率
    void scanDisplay(void)
    {
        spi.beginTransaction(spi_settings);
        digitalWrite(PIN_MATRIX_CS, LOW);
        delayMicroseconds(5);

        // 从机按大端收 uint16_t，ESP32 是小端，发之前逐像素换字节序
        for (int y = 0; y < matrix::HEIGHT; y++)
            for (int x = 0; x < matrix::WIDTH; x++)
                framebuffer[y][x] = __builtin_bswap16(framebuffer[y][x]);

        spi.transferBytes((uint8_t *)framebuffer, nullptr, sizeof(framebuffer));

        for (int y = 0; y < matrix::HEIGHT; y++)
            for (int x = 0; x < matrix::WIDTH; x++)
                framebuffer[y][x] = __builtin_bswap16(framebuffer[y][x]);

        delayMicroseconds(2);
        digitalWrite(PIN_MATRIX_CS, HIGH);
        spi.endTransaction();
    }

    void setFont(const uint8_t *u8g2_font)
    {
        font = u8g2_font;
        readFontInfo();
    }

    void setColor(uint16_t color)
    {
        fg_color = color;
    }

    int16_t drawUTF8(int16_t x, int16_t y, const char *str)
    {
        int16_t sum = 0;
        while (*str)
        {
            uint16_t encoding = utf8Decode(str);
            if (encoding == 0)
                break;
            decode.target_x = x;
            decode.target_y = y;
            const uint8_t *glyph = getGlyphData(encoding);
            if (glyph)
            {
                int16_t delta = decodeGlyph(glyph);
                x += delta;
                sum += delta;
            }
        }
        return sum;
    }

    void drawUTF8Centered(int16_t y, const char *str)
    {
        int16_t x = (canvas.width() - utf8Width(str)) / 2;
        drawUTF8(x < 0 ? 0 : x, y, str);
    }

    // 彩虹旋转边框：6 色循环
    void drawRainbow(uint8_t anim_state)
    {
        constexpr uint16_t colors[] = {RED, YELLOW, GREEN, CYAN, BLUE, FUCHSIA};
        constexpr int num_colors = sizeof(colors) / sizeof(colors[0]);
        for (int pos = 0; pos < PERIMETER; pos++)
            borderPixel(pos, colors[(pos / 4 + anim_state) % num_colors]);
    }

    void drawRed(uint8_t anim_state)
    {
        dashedBorder(RED, anim_state);
    }

    void drawGreen(uint8_t anim_state)
    {
        dashedBorder(GREEN, anim_state);
    }

    void drawBlue(uint8_t anim_state)
    {
        dashedBorder(BLUE, anim_state);
    }

    // 三只电表竖排：每只一行功率、一行电量
    void update(void)
    {
        static uint32_t last_ms = 0;
        static bool ready = false;

        const uint32_t now = millis();
        if (ready && now - last_ms < 100)
            return;
        last_ms = now;

        if (!ready)
        {
            setFont(u8g2_font_4x6_tf);
            ready = true;
        }

        canvas.fillScreen(BLACK);
        for (size_t i = 0; i < ztw_ddsu666::meter_count; i++)
        {
            const ztw_ddsu666::Data &meter = ztw_ddsu666::meters[i];
            const int16_t y = i * 21; // 三块均分 64px 高度
            char power[16], energy[16];

            if (!meter.online)
                snprintf(power, sizeof(power), "%u:--W", meter.address);
            else if (meter.power_w < 1000)
                snprintf(power, sizeof(power), "%u:%.1fW", meter.address, meter.power_w);
            else
                snprintf(power, sizeof(power), "%u:%.0fW", meter.address, meter.power_w);

            if (!meter.online)
                snprintf(energy, sizeof(energy), "--kWh");
            else if (meter.energy_kwh < 1000)
                snprintf(energy, sizeof(energy), "%.1fkWh", meter.energy_kwh);
            else
                snprintf(energy, sizeof(energy), "%.0fkWh", meter.energy_kwh);

            setColor(meter.online ? GREEN : RED);
            drawUTF8(0, y + 8, power);

            setColor(WHITE);
            drawUTF8(0, y + 17, energy);
        }

        scanDisplay();
    }
}
