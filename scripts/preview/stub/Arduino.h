#pragma once
// 宿主机跑矩阵屏渲染时替换 Arduino 核心，只补 Adafruit_GFX 和 matrix_gfx 用到的部分
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <algorithm>
#include <Print.h>

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define PROGMEM

typedef bool boolean;
typedef uint8_t byte;
typedef std::string String;
class __FlashStringHelper;

inline float radians(float deg) { return deg * float(M_PI) / 180.0f; }

using std::max;
using std::min;

inline unsigned long millis() { return 0; }
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline void delayMicroseconds(unsigned int) {}
