#pragma once

#include <stdint.h>

struct IbusFrame {
    uint16_t ch[14];
    bool failsafeFlag;
};

class IbusParser {
public:
    bool push(uint8_t byte, IbusFrame& out);

private:
    uint8_t buffer[32]{};
    uint8_t size = 0;
};
