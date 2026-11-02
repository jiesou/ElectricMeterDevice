#include "hal/matrix.h"

namespace matrix
{
    void init(void)
    {
        begin();
    }

    void update(void)
    {
        static uint32_t lastUpdate = 0;
        uint32_t now = millis();
        if (now - lastUpdate < 200)
            return;
        lastUpdate = now;

        canvas.fillScreen(0);
        drawContent();
        drawRainbow();

        scanDisplay();
    }
}
