#pragma once
#include <stdint.h>
#include <driver/ledc.h>

// ============ 引脚定义 ============
// GPIO0/1/2/3 是 strapping + UART0，别占用
#define PIN_BT0 0 // Boot button
#define PIN_BUZZER 19
#define PIN_LED 2

#define PIN_UART2_TX 33
#define PIN_UART2_RX 32

#define PIN_RELAY_1 25
#define PIN_RELAY_2 26
#define PIN_RELAY_3 27

#define PIN_BUTTON_LAST 34
#define PIN_BUTTON_NEXT 35

// 矩阵屏 HUB75 走 SPI，只写不读
#define PIN_MATRIX_CS 5
#define PIN_MATRIX_SCK 18
#define PIN_MATRIX_MOSI 23
