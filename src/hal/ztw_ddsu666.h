#pragma once
#include <stdint.h>

// 正泰 DDSU666 单相导轨式电能表，RS485 在端子 24(A)/25(B)
// Modbus-RTU、地址 1、9600 8N1，寄存器表见 .agents/notes/research/电能表数据手册/ZTW0.464.0090
namespace ztw_ddsu666 {
    constexpr uint32_t BAUD = 9600;

    struct Reading
    {
        float voltage_v;
        float current_a;
        float power_w;
        float reactive_var;
        float apparent_va;
        float power_factor;
        float frequency_hz;
        float energy_kwh;
    };

    extern Reading reading;
    extern bool online; // 最近一轮读取是否成功

    void init(void);
    void update(void);
}
