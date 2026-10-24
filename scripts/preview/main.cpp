// 宿主机渲染一帧矩阵屏，输出 PPM 给 scripts/preview.py 转 PNG
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "hal/matrix.h"

namespace entities
{
    std::vector<Entity> data;
    std::deque<Entity> queue;
    int peopleCount = 2;
    char roomName[24] = "450";
}

namespace ztw_ddsu666
{
    Data meters[METER_COUNT] = {
        {true, 1524, 123.4f},
        {true, 0, 0},
        {true, 0, 0},
    };
    const char *ENTITY_METER_ADDR_MAP[METER_COUNT] = {"light-meter", "kettle-meter", "dryer-meter"};
}

int main(int argc, char **argv)
{
    if (argc > 1)
        snprintf(entities::roomName, sizeof(entities::roomName), "%s", argv[1]);
    if (argc > 2)
        entities::peopleCount = atoi(argv[2]);
    if (argc > 3)
        ztw_ddsu666::meters[0].power_w = atof(argv[3]);
    if (argc > 4)
        ztw_ddsu666::meters[0].energy_kwh = atof(argv[4]);

    matrix::begin();
    matrix::canvas.fillScreen(matrix::BLACK);
    matrix::drawContent();
    matrix::drawRainbow();

    // rotation=1：逻辑坐标 x 向右、y 向下，对应原始 64x32 缓冲的 (63-y, x)
    constexpr int SCALE = 10;
    const int cols = matrix::canvas.width();   // 竖屏 32
    const int rows = matrix::canvas.height();  // 竖屏 64
    const uint16_t *buffer = matrix::canvas.getBuffer();

    printf("P6\n%d %d\n255\n", cols * SCALE, rows * SCALE);
    for (int y = 0; y < rows; y++)
    {
        static uint8_t line[64 * SCALE * 3];
        for (int x = 0; x < cols; x++)
        {
            const uint16_t c = buffer[(matrix::WIDTH - 1 - y) + x * matrix::WIDTH];
            const uint8_t rgb[3] = {
                uint8_t(((c >> 11) & 0x1F) * 255 / 31),
                uint8_t(((c >> 5) & 0x3F) * 255 / 63),
                uint8_t((c & 0x1F) * 255 / 31),
            };
            for (int i = 0; i < SCALE; i++)
                memcpy(&line[(x * SCALE + i) * 3], rgb, 3);
        }
        for (int i = 0; i < SCALE; i++)
            fwrite(line, 1, cols * SCALE * 3, stdout);
    }
    return 0;
}
