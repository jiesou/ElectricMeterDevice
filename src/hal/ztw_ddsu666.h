#pragma once
#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "pins.h"

// 寄存器表见 .agents/notes/research/电能表数据手册/ZTW0.464.0090
namespace ztw_ddsu666
{
    constexpr size_t METER_COUNT = 3;

    constexpr uint8_t FUNCTION_READ = 0x03;
    constexpr uint16_t POWER_REG = 0x2004;  // 瞬时有功功率，寄存器里单位是 kW
    constexpr uint16_t ENERGY_REG = 0x4000; // 有功总电能，kWh
    constexpr uint8_t RESPONSE_SIZE = 9;    // 地址 功能 字节数 4 字节 float CRC
    constexpr uint8_t ECHO_SIZE = 0;        // 发送时接收被关掉，听不到自己发的 8 字节；老模块 RE 常开，这里要填 8
    constexpr uint8_t FRAME_SIZE = ECHO_SIZE + RESPONSE_SIZE;
    constexpr uint32_t GAP_MS = 30;         // 帧间静默
    constexpr uint32_t TIMEOUT_MS = 500;    // 等不到回帧就跳过这只表

    // EN 高电平抢总线发，低电平交还总线收；模块反相就把这两个换过来
    constexpr uint8_t TX_LEVEL = HIGH;
    constexpr uint8_t RX_LEVEL = LOW;

    struct Data
    {
        bool online;
        float power_w;
        float energy_kwh;
    };
    inline Data meters[METER_COUNT] = {};
    inline const char *ENTITY_METER_ADDR_MAP[METER_COUNT] = {"light-meter", "kettle-meter", "dryer-meter"}; // 表地址就是下标 + 1，这里填它在实体表里的 id
    inline void (*onDataCallback)(size_t index) = nullptr;

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

    inline void send_request(uint8_t addr, uint16_t reg)
    {
        uint8_t request[8] = {addr, FUNCTION_READ, uint8_t(reg >> 8), uint8_t(reg), 0, 2};
        const uint16_t crc = crc16(request, 6);
        request[6] = crc & 0xFF; // CRC 低字节在前
        request[7] = crc >> 8;

        while (Serial2.available())
            Serial2.read();

        digitalWrite(PIN_RS485_EN, TX_LEVEL);
        Serial2.write(request, sizeof(request));
        Serial2.flush(); // 等最后一位真的发完才能放总线，早放会截断
        digitalWrite(PIN_RS485_EN, RX_LEVEL);
    }

    // 响应 9 字节：地址 功能 字节数 4 字节 float CRC
    inline bool parse(uint8_t addr, uint16_t reg, const uint8_t *frame)
    {
        if (frame[0] != addr || frame[1] != FUNCTION_READ || frame[2] != 4)
            return false;
        if (crc16(frame, 7) != uint16_t(frame[7] | frame[8] << 8))
            return false;

        Data &meter = meters[addr - 1];
        const float value = float_at(frame + 3);
        if (reg == POWER_REG)
            meter.power_w = value * 1000; // 寄存器里 P/Q/S 单位是 kW/kvar/kVA
        else
            meter.energy_kwh = value;
        return true;
    }

    // 把串口里能读的都读进来，返回是否攒够一帧
    inline bool read_frame(uint8_t *frame, uint8_t &size)
    {
        while (Serial2.available() && size < FRAME_SIZE)
            frame[size++] = Serial2.read();
        return size == FRAME_SIZE;
    }

    // 收完一帧：校验、填读数、决定下一帧读哪只表的哪个寄存器
    inline void finish(uint8_t &addr, uint16_t &reg, const uint8_t *frame, uint8_t size)
    {
        const bool ok = size == FRAME_SIZE && parse(addr, reg, frame + ECHO_SIZE);
        Data &meter = meters[addr - 1];

        if (!ok)
        {
            if (meter.online) // 掉线只报一次，免得每 500ms 刷一行日志
                Serial.printf("[ddsu666] addr=%u offline\n", addr);
            meter.online = false;
        }
        else if (reg == ENERGY_REG)
        {
            meter.online = true;
            if (onDataCallback)
                onDataCallback(addr - 1);
        }

        if (!ok || reg == ENERGY_REG)
        {
            addr = addr % METER_COUNT + 1; // 这只表读完了，换下一只
            reg = POWER_REG;
        }
        else
            reg = ENERGY_REG; // 功率读到了，接着读同一只表的电能
    }

    inline void on_data(void (*callback)(size_t index))
    {
        onDataCallback = callback;
    }

    inline void pollData(void)
    {
        static uint8_t addr = 1, rxSize = 0, rx[FRAME_SIZE];
        static uint16_t reg = POWER_REG;
        static bool waiting = false;
        static uint32_t sentMs = 0, lastFrameMs = 0;

        if (waiting)
        {
            if (!read_frame(rx, rxSize) && millis() - sentMs < TIMEOUT_MS)
                return;
            waiting = false;
            lastFrameMs = millis();
            finish(addr, reg, rx, rxSize);
            rxSize = 0;
        }

        if (millis() - lastFrameMs < GAP_MS)
            return;

        send_request(addr, reg);
        sentMs = millis();
        waiting = true;
    }

    void init(void);
    void update(void);
}