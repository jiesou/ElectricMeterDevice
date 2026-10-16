#pragma once
#include <stddef.h>
#include <stdint.h>

// 正泰 DDSU666 单相导轨式电能表，RS485 在端子 24(A)/25(B)
// Modbus-RTU、地址 1、9600 8N1，寄存器表见 .agents/notes/research/电能表数据手册/ZTW0.464.0090
namespace ztw_ddsu666 {
    constexpr uint32_t BAUD = 9600;

    struct Data
    {
        uint8_t address;
        bool online;
        float voltage_v;
        float current_a;
        float power_w;
        float reactive_var;
        float apparent_va;
        float power_factor;
        float frequency_hz;
        float energy_kwh;
    };

    constexpr size_t METER_COUNT = 3;
    extern Data meters[METER_COUNT];

    // 每只表读全一轮就回调一次，index 是 meters 下标
    void on_data(void (*callback)(size_t index));

    void init(void);
    void update(void);
}
