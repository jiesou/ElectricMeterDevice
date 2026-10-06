#pragma once
#include <stdint.h>

// 正泰 DDSU666 单相导轨式电能表，RS485 在端子 24(A)/25(B)
// 出厂默认 Modbus-RTU、地址 1、9600bps
namespace ztw_ddsu666 {
    // 电脑端 USB 转四八五 用同样的波特率打开
    constexpr uint32_t BAUD = 9600;

    void init(void);
    void update(void);
}