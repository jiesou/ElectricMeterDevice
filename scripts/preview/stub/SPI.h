#pragma once
// matrix_gfx 的 SPI 只在真机上发帧，宿主机渲染全部吞掉
#include <cstddef>
#include <cstdint>

constexpr int HSPI = 0;
constexpr int MSBFIRST = 1;
constexpr int SPI_MODE0 = 0;

class SPISettings
{
public:
    SPISettings(uint32_t, uint8_t, uint8_t) {}
};

class SPIClass
{
public:
    explicit SPIClass(int) {}
    void begin(int, int, int, int) {}
    void beginTransaction(SPISettings) {}
    void endTransaction() {}
    void transferBytes(const uint8_t *, uint8_t *, size_t) {}
};

extern SPIClass SPI;
