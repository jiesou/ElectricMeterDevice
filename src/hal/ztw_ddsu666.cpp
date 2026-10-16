#include "hal/ztw_ddsu666.h"
#include <Arduino.h>
#include "pins.h"

namespace
{
    // 表地址，登录 meters 的下标即数组下标
    constexpr uint8_t METER_ADDRESSES[ztw_ddsu666::METER_COUNT] = {1, 2, 3};
    constexpr uint8_t FUNCTION_READ = 0x03;
    constexpr uint32_t TIMEOUT_MS = 500;

    struct Block
    {
        uint16_t start;
        uint16_t count; // 寄存器个数，float 占 2 个
    };
    constexpr Block BLOCKS[] = {{0x2000, 16}, {0x4000, 2}}; // U..Freq，Ep
    constexpr size_t BLOCK_COUNT = sizeof(BLOCKS) / sizeof(BLOCKS[0]);

    uint8_t rx[5 + 2 * 16];
    size_t echoLeft = 0; // 485 模块会把发出去的字节回灌到 RX，先吃掉
    size_t rxSize = 0;
    size_t meterIndex = 0;
    size_t blockIndex = 0;
    bool waiting = false;
    uint32_t sentMs = 0;
    void (*dataCallback)(size_t index) = nullptr;

    uint16_t crc16(const uint8_t *data, size_t size)
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

    // 响应数据区第 word 个寄存器起的 float，大端
    float float_at(const uint8_t *data, size_t word)
    {
        const uint8_t *p = data + 2 * word;
        const uint32_t bits = uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
        float value;
        memcpy(&value, &bits, sizeof(value));
        return value;
    }

    void send_request(const Block &block)
    {
        uint8_t request[8] = {ztw_ddsu666::meters[meterIndex].address, FUNCTION_READ, uint8_t(block.start >> 8), uint8_t(block.start),
                              uint8_t(block.count >> 8), uint8_t(block.count)};
        const uint16_t crc = crc16(request, 6);
        request[6] = crc & 0xFF; // CRC 低字节在前
        request[7] = crc >> 8;

        while (Serial2.available())
            Serial2.read();
        Serial2.write(request, sizeof(request));
        echoLeft = sizeof(request);
        rxSize = 0;
        waiting = true;
        sentMs = millis();
    }

    // 帧完整后校验并解析，返回是否成功
    bool parse(const Block &block)
    {
        if (rx[0] != ztw_ddsu666::meters[meterIndex].address || rx[1] != FUNCTION_READ)
            return false;
        const size_t dataSize = 2 * block.count;
        const size_t frameSize = 5 + dataSize;
        if (rx[2] != dataSize || crc16(rx, frameSize - 2) != uint16_t(rx[frameSize - 2] | rx[frameSize - 1] << 8))
            return false;

        const uint8_t *data = rx + 3;
        if (block.start == 0x2000)
        {
            ztw_ddsu666::meters[meterIndex].voltage_v = float_at(data, 0);
            ztw_ddsu666::meters[meterIndex].current_a = float_at(data, 2);
            ztw_ddsu666::meters[meterIndex].power_w = float_at(data, 4) * 1000; // 寄存器里 P/Q/S 单位是 kW/kvar/kVA
            ztw_ddsu666::meters[meterIndex].reactive_var = float_at(data, 6) * 1000;
            ztw_ddsu666::meters[meterIndex].apparent_va = float_at(data, 8) * 1000;
            ztw_ddsu666::meters[meterIndex].power_factor = float_at(data, 10);
            ztw_ddsu666::meters[meterIndex].frequency_hz = float_at(data, 14);
        }
        else
        {
            ztw_ddsu666::meters[meterIndex].energy_kwh = float_at(data, 0);
        }
        return true;
    }

    // 掉线只报一次，免得每 132ms 刷一行日志把串口堵住
    void fail(void)
    {
        if (ztw_ddsu666::meters[meterIndex].online)
        {
            Serial.printf("[ddsu666] addr=%u offline\n", ztw_ddsu666::meters[meterIndex].address);
        }
        ztw_ddsu666::meters[meterIndex].online = false;
        waiting = false;
        blockIndex = 0;
        if (++meterIndex == ztw_ddsu666::METER_COUNT)
            meterIndex = 0;
    }
}

namespace ztw_ddsu666
{
    Data meters[ztw_ddsu666::METER_COUNT] = {};

    void init(void)
    {
        Serial2.begin(BAUD, SERIAL_8N1, PIN_UART2_RX, PIN_UART2_TX);
        for (size_t i = 0; i < ztw_ddsu666::METER_COUNT; i++)
            meters[i].address = METER_ADDRESSES[i];
    }

    void on_data(void (*callback)(size_t index))
    {
        dataCallback = callback;
    }

    void update(void)
    {
        const uint32_t now = millis();

        if (waiting)
        {
            while (Serial2.available() && rxSize < sizeof(rx))
            {
                const uint8_t b = Serial2.read();
                if (echoLeft)
                    echoLeft--;
                else
                    rx[rxSize++] = b;
            }

            const Block &block = BLOCKS[blockIndex];
            const bool isException = rxSize >= 2 && (rx[1] & 0x80);
            const size_t expected = isException ? 5 : 5 + 2 * block.count;

            if (rxSize >= 2 && rxSize >= expected)
            {
                if (isException)
                    return fail();
                if (!parse(block))
                    return fail();

                waiting = false;
                if (++blockIndex == BLOCK_COUNT)
                {
                    blockIndex = 0;
                    meters[meterIndex].online = true;
                    if (dataCallback)
                        dataCallback(meterIndex);
                    if (++meterIndex == ztw_ddsu666::METER_COUNT)
                        meterIndex = 0;
                }
            }
            else if (now - sentMs >= TIMEOUT_MS)
            {
                fail();
            }
            return;
        }

        // 上一帧一回来就接着发下一帧，不设人为间隔
        send_request(BLOCKS[blockIndex]);
    }
}
