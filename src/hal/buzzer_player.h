#pragma once
#include <cstddef>
#include <stdint.h>

namespace buzzer_player {
    struct Note
    {
        uint16_t frequency;
        uint16_t durationMs;
        uint16_t delayMs;
    };

    void init(void);
    void update(void);
    void play(const Note *notes, uint8_t count);
    void off(void);

    template <size_t N>
    inline void play(const Note (&notes)[N])
    {
        play(notes, static_cast<uint8_t>(N));
    }

    inline void ready(void)
    {
        static const Note notes[] = {
            {3000, 50, 0},
            {3000, 50, 50},
        };
        play(notes);
    }

    inline void tada(void)
    {
        static const Note notes[] = {
            {1000, 100, 0},
            {1500, 200, 0},
            {3000, 200, 0},
        };
        play(notes);
    }

    inline void warn(void)
    {
        static const Note notes[] = {
            {1000, 300, 0},
            {800, 200, 100},
            {600, 200, 100},
        };
        play(notes);
    }
}
