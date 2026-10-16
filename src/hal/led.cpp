#include "hal/led.h"
#include <Arduino.h>
#include <WiFi.h>
#include <driver/ledc.h>
#include "hal/ztw_ddsu666.h"
#include "wsclient.h"

namespace
{
    constexpr uint32_t BEAT_MS = 300;
    constexpr uint32_t ON_MS = 150;
    constexpr uint32_t PAUSE_MS = 1000;
    constexpr uint32_t BEATS = 3;
    constexpr uint32_t CYCLE_MS = BEATS * BEAT_MS + PAUSE_MS;

    constexpr ledc_mode_t LEDC_SPD = LEDC_LOW_SPEED_MODE;
    constexpr ledc_channel_t LEDC_CH = LEDC_CHANNEL_1;
    constexpr uint32_t DUTY_BRIGHT = 255;
    constexpr uint32_t DUTY_DIM = 6;
}

namespace led
{
    void init(void)
    {
        ledc_timer_config_t t = {};
        t.speed_mode = LEDC_SPD;
        t.timer_num = LEDC_TIMER_0;
        t.duty_resolution = LEDC_TIMER_8_BIT;
        t.freq_hz = 5000;
        t.clk_cfg = LEDC_AUTO_CLK;
        ESP_ERROR_CHECK(ledc_timer_config(&t));

        ledc_channel_config_t ch = {};
        ch.gpio_num = PIN_LED;
        ch.speed_mode = LEDC_SPD;
        ch.channel = LEDC_CH;
        ch.timer_sel = LEDC_TIMER_0;
        ESP_ERROR_CHECK(ledc_channel_config(&ch));
    }

    // 三拍依次对应 WiFi、服务器、485：通了亮一下，没通暗一下
    void update(void)
    {
        const bool connected[BEATS] = {
            WiFi.status() == WL_CONNECTED,
            wsclient::isServerConnected,
            ztw_ddsu666::meters[0].online,
        };

        const uint32_t t = millis() % CYCLE_MS;
        const uint32_t beat = t / BEAT_MS;
        const bool lit = beat < BEATS && t % BEAT_MS < ON_MS;
        const uint32_t duty = !lit ? 0 : connected[beat] ? DUTY_BRIGHT : DUTY_DIM;

        static uint32_t lastDuty = UINT32_MAX;
        if (duty == lastDuty)
            return;
        lastDuty = duty;
        ledc_set_duty(LEDC_SPD, LEDC_CH, duty);
        ledc_update_duty(LEDC_SPD, LEDC_CH);
    }
}
