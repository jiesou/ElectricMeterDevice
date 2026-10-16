#include "hal/ztw_ddsu666.h"
#include <Arduino.h>
#include "pins.h"

namespace
{
    constexpr uint8_t FUNCTION_READ = 0x03;
    constexpr uint16_t POWER_REG = 0x2004;  // 瞬时有功功率，寄存器里单位是 kW
    constexpr uint16_t ENERGY_REG = 0x4000; // 有功总电能，kWh
    constexpr uint8_t FRAME_SIZE = 8 + 9; // 回灌的请求 8 字节 + 响应 9 字节

    void (*onDataCallback)(size_t index) = nullptr;

    void send_request(uint8_t addr, uint16_t reg)
    {
        uint8_t request[8] = {addr, FUNCTION_READ, uint8_t(reg >> 8), uint8_t(reg), 0, 2};
        const uint16_t crc = ztw_ddsu666::crc16(request, 6);
        request[6] = crc & 0xFF; // CRC 低字节在前
        request[7] = crc >> 8;

        while (Serial2.available())
            Serial2.read();
        Serial2.write(request, sizeof(request));
    }

    // 响应 9 字节：地址 功能 字节数 4 字节 float CRC
    bool parse(uint8_t addr, uint16_t reg, const uint8_t *frame)
    {
        if (frame[0] != addr || frame[1] != FUNCTION_READ || frame[2] != 4)
            return false;
        if (ztw_ddsu666::crc16(frame, 7) != uint16_t(frame[7] | frame[8] << 8))
            return false;

        ztw_ddsu666::Data &meter = ztw_ddsu666::meters[addr - 1];
        const float value = ztw_ddsu666::float_at(frame + 3);
        if (reg == POWER_REG)
            meter.power_w = value * 1000; // 寄存器里 P/Q/S 单位是 kW/kvar/kVA
        else
            meter.energy_kwh = value;
        return true;
    }
}

namespace ztw_ddsu666
{
    Data meters[METER_COUNT] = {};

    void init(void)
    {
        Serial2.begin(9600, SERIAL_8N1, PIN_UART2_RX, PIN_UART2_TX);
    }

    void on_data(void (*callback)(size_t index))
    {
        onDataCallback = callback;
    }

    void update(void)
    {
        static uint8_t addr = 1;         // 表地址就是下标 + 1
        static uint16_t reg = POWER_REG; // 每只表先读功率，再读电能
        static uint8_t rx[FRAME_SIZE];   // 485 模块会把发出去的 8 字节回灌到 RX
        static uint8_t rxSize = 0;
        static uint32_t sentMs = 0;

        if (!sentMs) // 上电第一帧
        {
            send_request(addr, reg);
            sentMs = millis();
            return;
        }

        while (Serial2.available() && rxSize < FRAME_SIZE)
            rx[rxSize++] = Serial2.read();

        const bool full = rxSize == FRAME_SIZE;
        rxSize = 0;
        if (!full && millis() - sentMs < 500) // timeout
            return;

        const bool success = parse(addr, reg, rx + 8);
        if (!full || !success)
        {
            Data &meter = meters[addr - 1];
            if (meter.online) // 掉线只报一次，免得每 500ms 刷一行日志
                Serial.printf("[ddsu666] addr=%u offline\n", addr);
            meter.online = false;
            addr = addr % METER_COUNT + 1;
            reg = POWER_REG;
        }
        else if (reg == POWER_REG)
            reg = ENERGY_REG; // 功率读到了，接着读这只表的电能
        else
        {
            meters[addr - 1].online = true;
            if (onDataCallback)
                onDataCallback(addr - 1);
            addr = addr % METER_COUNT + 1;
            reg = POWER_REG;
        }
    }
}
