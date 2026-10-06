#include "hal/ztw_ddsu666.h"
#include <Arduino.h>
#include "pins.h"

namespace
{
    constexpr uint32_t INTERVAL_MS = 2000;
    constexpr const char *PAYLOADS[] = {"1\r\n", "Hello World\r\n"};
    constexpr uint8_t PAYLOAD_COUNT = sizeof(PAYLOADS) / sizeof(PAYLOADS[0]);

    char line[64];
    size_t size = 0;
    uint32_t lastSendMs = 0;
    uint8_t next = 0;
}

namespace ztw_ddsu666
{
    void init(void)
    {
        Serial2.begin(BAUD, SERIAL_8N1, PIN_UART2_RX, PIN_UART2_TX);
    }

    void update(void)
    {
        const auto now = millis();

        while (Serial2.available())
        {
            const auto c = static_cast<char>(Serial2.read());
            if (c == '\n')
            {
                line[size] = '\0';
                Serial.printf("[%lu ms][ddsu666] rx: %s\n", now, line);
                size = 0;
            }
            else if (size < sizeof(line) - 1)
            {
                line[size++] = c;
            }
        }

        if (now - lastSendMs < INTERVAL_MS)
            return;
        lastSendMs = now;

        const auto *payload = PAYLOADS[next++ % PAYLOAD_COUNT];
        Serial2.print(payload);
        Serial.printf("[%lu ms][ddsu666] tx: %s", now, payload);
    }
}