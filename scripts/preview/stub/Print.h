#pragma once
// Adafruit_GFX 继承 Print，这里只留虚接口
#include <cstddef>
#include <cstdint>
#include <string>

class Print
{
public:
    virtual ~Print() {}
    virtual size_t write(uint8_t) { return 0; }
    virtual size_t write(const uint8_t *, size_t) { return 0; }
    size_t print(const char *) { return 0; }
    size_t print(const std::string &) { return 0; }
    size_t println(const char *) { return 0; }
};
