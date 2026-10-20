#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// 寄存器表见 .agents/notes/research/电能表数据手册/ZTW0.464.0090
namespace ztw_ddsu666 {
    constexpr size_t METER_COUNT = 3;
    struct Data
    {
        bool online;
        float power_w;
        float energy_kwh;
    };
    extern Data meters[METER_COUNT];
    extern const char *ENTITY_METER_ADDR_MAP[METER_COUNT]; // 表地址就是下标 + 1，这里填它在实体表里的 id

    void on_data(void (*callback)(size_t index));

    void init(void);
    void update(void);

    inline uint16_t crc16(const uint8_t *data, size_t size)
    {
        uint16_t crc = 0xFFFF;
        while (size--)
        {
            crc ^= *data++;
            for (int i = 0; i < 8; i++)
                crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
        return crc;
    }

    // 寄存器区开头 4 字节就是一个 float，大端
    inline float float_at(const uint8_t *data)
    {
        const uint32_t bits = uint32_t(data[0]) << 24 | uint32_t(data[1]) << 16 | uint32_t(data[2]) << 8 | data[3];
        float value;
        memcpy(&value, &bits, sizeof(value));
        return value;
    }
}
