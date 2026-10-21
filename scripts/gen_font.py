#!/usr/bin/env python3
"""从上游 u8g2 字体 .c 里挑出界面用到的字，生成 include/font_data.h。

构建时由 PlatformIO 调用（platformio.ini 的 extra_scripts），也可以手动跑：
    python3 scripts/gen_font.py

字形文件是上游原样的单字体 .c，放在 scripts/assets/ 里；换字体只需换文件 + 改 FONT：
    https://cdn.jsdelivr.net/gh/olikraus/u8g2@master/tools/font/build/single_font_files/<FONT>.c
字体列表：https://github.com/olikraus/u8g2/wiki/fntlistall

只收界面字符串里的汉字和数字（数字运行时格式化，固定收 0-9）：调试日志、WiFi 密码这类 ASCII 字符永远不会画到
屏上，收进来只是浪费 flash。界面上的字都写在 hal/matrix 里，所以只扫这两个文件。
缺字直接报错，不改文件。
"""

import re
import sys
from pathlib import Path

try:
    Import("env")  # PlatformIO 的 extra_scripts 会注入它，手动运行时没有
    ROOT = Path(env.subst("$PROJECT_DIR"))
    IN_BUILD = True
except NameError:
    ROOT = Path(__file__).resolve().parent.parent
    IN_BUILD = False

FONT = "u8g2_font_wqy12_t_gb2312"
FONT_C = ROOT / "scripts" / "assets" / f"{FONT}.c"
OUT = ROOT / "include" / "font_data.h"
UI_FILES = [ROOT / "src" / "hal" / "matrix.h", ROOT / "src" / "hal" / "matrix.cpp"]

# 12px 字：基线在行顶下 10 行（CJK 字高 11，u8g2 的 y 偏移 -1）
ASCENT = 10


class Bits:
    def __init__(self, data, pos):
        self.data, self.pos, self.bit = data, pos, 0

    def unsigned(self, cnt):
        if cnt == 0:
            return 0
        val = self.data[self.pos] >> self.bit
        bit = self.bit + cnt
        if bit >= 8:
            val |= self.data[self.pos + 1] << (8 - self.bit)
            self.pos += 1
            bit -= 8
        self.bit = bit
        return val & ((1 << cnt) - 1)

    def signed(self, cnt):
        return 0 if cnt == 0 else self.unsigned(cnt) - (1 << (cnt - 1))


class Font:
    """u8g2 字体格式：23 字节头 + 字形表 + unicode 索引，字形是逐位 RLE。"""

    def __init__(self, data):
        self.data = data
        self.bits_per_0, self.bits_per_1 = data[2], data[3]
        self.bpcw, self.bpch, self.bpcx, self.bpcy, self.bpcd = data[4:9]
        self.pos_upper, self.pos_lower, self.pos_unicode = self.word(17), self.word(19), self.word(21)

    def word(self, pos):
        return (self.data[pos] << 8) | self.data[pos + 1]

    def glyph_data(self, code):
        pos = 23
        if code <= 255:
            pos += self.pos_lower if code >= ord("a") else self.pos_upper if code >= ord("A") else 0
            while self.data[pos + 1]:
                if self.data[pos] == code:
                    return pos + 2
                pos += self.data[pos + 1]
            return None

        base = 23 + self.pos_unicode
        pos, i = 0, 0
        while True:
            pos += self.word(base + i * 4)
            if self.word(base + i * 4 + 2) >= code:
                break
            i += 1
        glyph = base + pos
        while True:
            current = (self.data[glyph] << 8) | self.data[glyph + 1]
            if current == 0:
                return None
            if current == code:
                return glyph + 3
            glyph += self.data[glyph + 2]

    def glyph(self, code):
        pos = self.glyph_data(code)
        if pos is None:
            return None
        bits = Bits(self.data, pos)
        w = bits.unsigned(self.bpcw)
        h = bits.unsigned(self.bpch)
        x = bits.signed(self.bpcx)
        y = bits.signed(self.bpcy)
        advance = bits.signed(self.bpcd)

        rows = [[0] * w for _ in range(h)] if w > 0 else []
        lx = ly = 0
        if w > 0:
            while True:
                zero = bits.unsigned(self.bits_per_0)
                one = bits.unsigned(self.bits_per_1)
                # 重复位为 1 时同一对游程再画一遍，不是重新读一对
                while True:
                    for length, ink in ((zero, 0), (one, 1)):
                        count, lx0, ly0 = length, lx, ly
                        while True:
                            room = w - lx0
                            take = min(count, room)
                            if ink and ly0 < h:
                                for i in range(take):
                                    rows[ly0][lx0 + i] = 1
                            if count < room:
                                break
                            count -= room
                            lx0, ly0 = 0, ly0 + 1
                        lx, ly = lx0 + count, ly0
                    if not bits.unsigned(1):
                        break
                if ly >= h:
                    break
        return {"w": w, "h": h, "x": x, "y": y, "advance": advance, "rows": rows}


def load_font(path):
    """U8G2_FONT_SECTION 是一串 C 字符串，字节用八进制转义写出来。"""
    text = path.read_text(encoding="utf-8", errors="ignore")
    start = text.index(f"const uint8_t {FONT}[")
    size = int(text[start + len(f"const uint8_t {FONT}["):text.index("]", start)])
    body = text[text.index('"', text.index("=", start)):]

    out = bytearray()
    i, quoted = 0, False
    while i < len(body) and len(out) < size:
        c = body[i]
        if not quoted:
            quoted = c == '"'
            i += 1
            continue
        if c == '"':
            quoted = False
            i += 1
            continue
        if c != "\\":
            out.append(ord(c))
            i += 1
            continue
        i += 1
        if body[i] in "01234567":
            j = i
            while j < len(body) and j < i + 3 and body[j] in "01234567":
                j += 1
            out.append(int(body[i:j], 8))
            i = j
            continue
        out.append(ord(body[i]))
        i += 1
    return Font(bytes(out))


def used_chars():
    # 数字是 snprintf 格式化出来的，字面量里只有 "301" 里的那几个，所以固定全收 0-9
    chars = set("0123456789")
    for path in UI_FILES:
        text = path.read_text(encoding="utf-8", errors="ignore")
        text = re.sub(r"^\s*#\s*include[^\n]*", "", text, flags=re.M)
        text = re.sub(r"//[^\n]*", "", text)
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        for literal in re.findall(r'"((?:[^"\\]|\\.)*)"', text):
            for char in re.sub(r"\\.", "", literal):
                if not char.isascii() or char.isdigit():
                    chars.add(char)
    return sorted(chars)


# 输出照 Adafruit GFX 的 GFXglyph 排（bitmapOffset/width/height/xAdvance/xOffset/yOffset），
# 只多一个 16 位 codepoint：GFXfont 要求码位连续，数字 0x30 到汉字 0x4E00 空太多，索引表会白占。
def render(glyphs):
    bitmap = []
    entries = []
    for char, glyph in glyphs:
        offset = len(bitmap)
        for row in glyph["rows"]:
            packed = bytearray((glyph["w"] + 7) // 8)
            for i, on in enumerate(row):
                if on:
                    packed[i // 8] |= 0x80 >> (i % 8)
            bitmap += packed
        entries.append(
            f"        {{0x{ord(char):04X}, {offset}, {glyph['w']}, {glyph['h']}, "
            f"{glyph['advance']}, {glyph['x']}, {ASCENT - glyph['h'] - glyph['y']}}},"
        )

    out = [
        "// Generated by scripts/gen_font.py",
        "#pragma once",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "namespace font",
        "{",
        "    struct Glyph",
        "    {",
        "        uint16_t codepoint;",
        "        uint16_t bitmapOffset;",
        "        uint8_t width;",
        "        uint8_t height;",
        "        uint8_t xAdvance;",
        "        int8_t xOffset;",
        "        int8_t yOffset;",
        "    };",
        "",
        "    inline const uint8_t BITMAP[] = {",
    ]
    for i in range(0, len(bitmap), 12):
        out.append("        " + ", ".join(f"0x{b:02X}" for b in bitmap[i:i + 12]) + ",")
    out += [
        "    };",
        "",
        "    inline const Glyph GLYPHS[] = {",
        *entries,
        "    };",
        "    constexpr size_t COUNT = sizeof(GLYPHS) / sizeof(GLYPHS[0]);",
        "}",
        "",
    ]
    return "\n".join(out)


def main():
    font = load_font(FONT_C)
    chars = used_chars()
    glyphs = []
    missing = []
    for char in chars:
        glyph = font.glyph(ord(char))
        if glyph:
            glyphs.append((char, glyph))
        else:
            missing.append(char)
    if missing:
        sys.exit(f"字库缺字：{' '.join(missing)}（{FONT_C.name} 里没有）")

    text = render(glyphs)
    if OUT.exists() and OUT.read_text(encoding="utf-8") == text:
        return
    OUT.write_text(text, encoding="utf-8")
    print(f"[font] {len(chars)} 个字 -> {OUT.relative_to(ROOT)}")


if IN_BUILD or __name__ == "__main__":
    main()
