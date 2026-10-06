#include "hal/buzzer.h"
#include <driver/ledc.h>
#include <esp_timer.h>

namespace
{
    constexpr ledc_mode_t LEDC_SPD = LEDC_LOW_SPEED_MODE;
    constexpr ledc_timer_t LEDC_TMR = LEDC_TIMER_1;
    constexpr ledc_channel_t LEDC_CH = LEDC_CHANNEL_0;
    constexpr uint8_t LEDC_BITS = 8;
    constexpr uint32_t LEDC_DUTY = (1U << LEDC_BITS) / 2U;

    void tone(uint16_t frequency)
    {
        ESP_ERROR_CHECK(ledc_set_freq(LEDC_SPD, LEDC_TMR, frequency * buzzer::volume / 64));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPD, LEDC_CH, LEDC_DUTY * buzzer::volume / 128));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPD, LEDC_CH));
    }

    void stop_now()
    {
        ledc_stop(LEDC_SPD, LEDC_CH, 0);
    }

    struct Note
    {
        uint16_t frequency = 0;
        uint16_t durationMs = 0;
        uint16_t delayMs = 0;
    };

    Note note = {};
    bool noteActive = false;
    esp_timer_handle_t timer = nullptr;

    void clear_note()
    {
        note = {};
        noteActive = false;
    }

    void start_timer(uint16_t durationMs)
    {
        if (!timer || durationMs == 0)
            return;
        ESP_ERROR_CHECK(esp_timer_start_once(timer, static_cast<uint64_t>(durationMs) * 1000ULL));
    }

    void start_note(void)
    {
        tone(note.frequency);
        noteActive = true;
        start_timer(note.durationMs);
    }

    void update(void *)
    {
        if (noteActive)
        {
            stop_now();
            clear_note();
            return;
        }

        if (note.delayMs > 0)
        {
            const auto delayMs = note.delayMs;
            note.delayMs = 0;
            start_timer(delayMs);
            return;
        }

        if (note.frequency == 0)
            return;

        start_note();
    }
}

namespace buzzer
{
    uint8_t volume = 50;
    void init(void)
    {
        gpio_set_direction(static_cast<gpio_num_t>(PIN_BUZZER), GPIO_MODE_OUTPUT);
        gpio_set_level(static_cast<gpio_num_t>(PIN_BUZZER), 0);

        ledc_timer_config_t t = {};
        t.speed_mode = LEDC_SPD;
        t.timer_num = LEDC_TMR;
        t.duty_resolution = static_cast<ledc_timer_bit_t>(LEDC_BITS);
        t.freq_hz = 1000;
        t.clk_cfg = LEDC_AUTO_CLK;
        ESP_ERROR_CHECK(ledc_timer_config(&t));

        ledc_channel_config_t ch = {};
        ch.gpio_num = static_cast<gpio_num_t>(PIN_BUZZER);
        ch.speed_mode = LEDC_SPD;
        ch.channel = LEDC_CH;
        ch.intr_type = LEDC_INTR_DISABLE;
        ch.timer_sel = LEDC_TMR;
        ESP_ERROR_CHECK(ledc_channel_config(&ch));

        if (!timer)
        {
            esp_timer_create_args_t a = {};
            a.callback = update;
            a.name = "buzzer";
            ESP_ERROR_CHECK(esp_timer_create(&a, &timer));
        }

        off();
    }

    void on(uint16_t frequency, uint16_t durationMs)
    {
        on_for(frequency, durationMs, 0);
    }

    void on_for(uint16_t frequency, uint16_t durationMs, uint16_t delayMs)
    {
        off();

        note.frequency = frequency;
        note.durationMs = durationMs;
        note.delayMs = delayMs;
        if (delayMs > 0)
        {
            update(nullptr);
            return;
        }

        start_note();
    }

    void on_when(uint16_t frequency, uint16_t delayMs)
    {
        on_for(frequency, 0, delayMs);
    }

    void off(void)
    {
        if (!timer)
        {
            stop_now();
            clear_note();
            return;
        }
        if (esp_timer_is_active(timer))
            ESP_ERROR_CHECK(esp_timer_stop(timer));
        clear_note();
        stop_now();
    }
}