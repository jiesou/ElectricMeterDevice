#include "hal/matrix.h"

namespace matrix
{
    void init(void)
    {
        rgb_matrix::init();
    }

    void update(void)
    {
        static uint32_t lastUpdate = 0;
        uint32_t now = millis();
        if (now - lastUpdate < 200)
            return;
        lastUpdate = now;

        rgb_matrix::canvas.fillScreen(0);
        drawRainbow();
        drawContent();
        rgb_matrix::scanDisplay();
    }
}
