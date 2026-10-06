#include "hal/buzzer_player.h"
#include "hal/buzzer.h"
#include <esp_timer.h>

namespace
{
    struct State
    {
        const buzzer_player::Note *notes = nullptr;
        uint8_t size = 0;
        uint8_t index = 0;
    } state;

    esp_timer_handle_t timer = nullptr;

    void clear()
    {
        state.notes = nullptr;
        state.size = 0;
        state.index = 0;
    }
}

namespace buzzer_player
{
    void init(void)
    {
        if (timer)
            return;

        esp_timer_create_args_t a = {};
        a.callback = [](void *) { update(); };
        a.name = "buzzer_player";
        ESP_ERROR_CHECK(esp_timer_create(&a, &timer));
    }

    void update(void)
    {
        if (state.index >= state.size)
        {
            clear();
            return;
        }

        const auto &note = state.notes[state.index++];
        buzzer::on_for(note.frequency, note.durationMs, note.delayMs);

        if (note.durationMs == 0)
        {
            clear();
            return;
        }

        const uint32_t totalMs = static_cast<uint32_t>(note.delayMs) + note.durationMs;
        ESP_ERROR_CHECK(esp_timer_start_once(timer, static_cast<uint64_t>(totalMs) * 1000ULL));
    }

    void play(const Note *notes, uint8_t count)
    {
        off();
        if (!timer || !notes || count == 0)
            return;
        state.notes = notes;
        state.size = count;
        update();
    }

    void off(void)
    {
        if (timer && esp_timer_is_active(timer))
            ESP_ERROR_CHECK(esp_timer_stop(timer));
        clear();
        buzzer::off();
    }
}