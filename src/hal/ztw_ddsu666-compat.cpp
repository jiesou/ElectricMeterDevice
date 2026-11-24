// 备份：现场自己写 ztw_ddsu666.cpp 时，在文件顶上 #define USE_COMPAT 就换成这份
// 就是重构前那版：一个 update() 从头写到尾，不拆 pollData
#include "hal/ztw_ddsu666.h"
#include <driver/gpio.h>

namespace ztw_ddsu666
{
    void init(void)
    {
        gpio_set_level(static_cast<gpio_num_t>(PIN_RS485_EN), 0);
        gpio_set_direction(static_cast<gpio_num_t>(PIN_RS485_EN), GPIO_MODE_OUTPUT);
        Serial2.begin(9600, SERIAL_8N1, PIN_UART2_RX, PIN_UART2_TX);
    }

    void update(void)
    {
        static uint8_t addr = 1;         // 表地址就是下标 + 1
        static uint16_t reg = POWER_REG; // 每只表先读功率，再读电能
        static uint8_t rx[FRAME_SIZE];
        static uint8_t rxSize = 0;
        static bool waiting = false;
        static uint32_t sentMs = 0;
        static uint32_t lastFrameMs = 0;

        if (waiting)
        {
            while (Serial2.available() && rxSize < FRAME_SIZE)
                rx[rxSize++] = Serial2.read();

            const bool full = rxSize == FRAME_SIZE;
            if (!full && millis() - sentMs < TIMEOUT_MS) // timeout
                return;                                  // 回帧还没齐，先让别的活跑

            rxSize = 0;
            waiting = false;
            lastFrameMs = millis(); // 帧间静默从这一帧收完算起，不占着 CPU 等

            const bool success = parse(addr, reg, rx + ECHO_SIZE);
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

        if (millis() - lastFrameMs < GAP_MS)
            return;

        send_request(addr, reg);
        sentMs = millis();
        waiting = true;
    }
}