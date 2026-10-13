#include "hal/ztw_ddsu666.h"
#include <Arduino.h>
#include "pins.h"

namespace
{
    constexpr uint8_t ADDRESS = 1;
    constexpr uint8_t FUNCTION_READ = 0x03;
    constexpr uint32_t POLL_MS = 1000;
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
    size_t blockIndex = 0;
    bool waiting = false;
    uint32_t sentMs = 0;
    uint32_t pollMs = 0;

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
        uint8_t request[8] = {ADDRESS, FUNCTION_READ, uint8_t(block.start >> 8), uint8_t(block.start),
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
        if (rx[0] != ADDRESS || rx[1] != FUNCTION_READ)
        {
            Serial.printf("[ddsu666] bad response: addr=%u fn=0x%02x\n", rx[0], rx[1]);
            return false;
        }
        const size_t dataSize = 2 * block.count;
        const size_t frameSize = 5 + dataSize;
        if (rx[2] != dataSize || crc16(rx, frameSize - 2) != uint16_t(rx[frameSize - 2] | rx[frameSize - 1] << 8))
        {
            Serial.println("[ddsu666] bad byte count or CRC");
            return false;
        }

        const uint8_t *data = rx + 3;
        if (block.start == 0x2000)
        {
            ztw_ddsu666::reading.voltage_v = float_at(data, 0);
            ztw_ddsu666::reading.current_a = float_at(data, 2);
            ztw_ddsu666::reading.power_w = float_at(data, 4) * 1000; // 寄存器里 P/Q/S 单位是 kW/kvar/kVA
            ztw_ddsu666::reading.reactive_var = float_at(data, 6) * 1000;
            ztw_ddsu666::reading.apparent_va = float_at(data, 8) * 1000;
            ztw_ddsu666::reading.power_factor = float_at(data, 10);
            ztw_ddsu666::reading.frequency_hz = float_at(data, 14);
        }
        else
        {
            ztw_ddsu666::reading.energy_kwh = float_at(data, 0);
        }
        return true;
    }

    void fail(void)
    {
        ztw_ddsu666::online = false;
        waiting = false;
        blockIndex = 0;
    }
}

namespace ztw_ddsu666
{
    Reading reading = {};
    bool online = false;

    void init(void)
    {
        Serial2.begin(BAUD, SERIAL_8N1, PIN_UART2_RX, PIN_UART2_TX);
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
                {
                    Serial.printf("[ddsu666] exception code %u\n", rx[2]);
                    return fail();
                }
                if (!parse(block))
                    return fail();

                waiting = false;
                if (++blockIndex == BLOCK_COUNT)
                {
                    blockIndex = 0;
                    online = true;
                    Serial.printf("[ddsu666] U=%.2fV I=%.3fA P=%.1fW Q=%.1fvar S=%.1fVA PF=%.3f F=%.2fHz Ep=%.3fkWh\n",
                                  reading.voltage_v, reading.current_a, reading.power_w, reading.reactive_var,
                                  reading.apparent_va, reading.power_factor, reading.frequency_hz, reading.energy_kwh);
                }
            }
            else if (now - sentMs >= TIMEOUT_MS)
            {
                Serial.printf("[ddsu666] timeout, got %u bytes\n", rxSize);
                fail();
            }
            return;
        }

        if (blockIndex == 0)
        {
            if (now - pollMs < POLL_MS)
                return;
            pollMs = now;
        }
        send_request(BLOCKS[blockIndex]);
    }
}
