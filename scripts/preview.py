#!/usr/bin/env python3
"""宿主机渲染矩阵屏，生成 PNG 预览。

    python3 scripts/preview.py [房名] [人数] [总功率W] [总电能kWh]

把 hal/matrix.h 的 drawContent 原样编译到宿主机跑一遍，输出 32x64 竖屏 PNG。
"""

import struct
import subprocess
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BIN = Path("/tmp/matrix-preview")


def png(path, width, height, rgb):
    raw = b"".join(b"\x00" + rgb[y * width * 3 : (y + 1) * width * 3] for y in range(height))

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw, 9))
        + chunk(b"IEND", b"")
    )


def main():
    cmd = [
        "g++", "-std=gnu++17", "-O2", "-DARDUINO=100",
        "-I", str(ROOT / "scripts/preview/stub"),
        "-I", str(ROOT / "src"),
        "-I", str(ROOT / "include"),
        "-I", str(ROOT / "lib/Adafruit_GFX/src"),
        str(ROOT / "scripts/preview/main.cpp"),
        str(ROOT / "src/hal/matrix_gfx.cpp"),
        str(ROOT / "lib/Adafruit_GFX/src/Adafruit_GFX.cpp"),
        "-o", str(BIN),
    ]
    subprocess.run(cmd, check=True)

    ppm = subprocess.run([str(BIN), *sys.argv[1:]], check=True, capture_output=True).stdout
    header, _, body = ppm.partition(b"\n255\n")
    width, height = map(int, header.split(b"\n")[1].split())
    path = Path("/tmp/matrix-preview.png")
    png(path, width, height, body)
    print(path)


if __name__ == "__main__":
    main()
