#include "ibus.h"

bool IbusParser::push(uint8_t byte, IbusFrame& out) {
    if (size == 0 && byte != 0x20) return false;
    if (size == 1 && byte != 0x40) {
        size = byte == 0x20 ? 1 : 0;
        return false;
    }
    buffer[size++] = byte;
    if (size < 32) return false;

    uint16_t checksum = 0xffff;
    for (int i = 0; i < 30; ++i) checksum -= buffer[i];
    if (checksum == (buffer[30] | (static_cast<uint16_t>(buffer[31]) << 8))) {
        out.failsafeFlag = false;
        for (int i = 0; i < 14; ++i) {
            uint16_t raw = buffer[2 + i * 2] | (static_cast<uint16_t>(buffer[3 + i * 2]) << 8);
            out.ch[i] = raw & 0x0fff;
            if (i < 5 && (raw & 0x2000)) out.failsafeFlag = true;
        }
        size = 0;
        return true;
    }

    uint8_t start = 1;
    while (start < 31 && !(buffer[start] == 0x20 && buffer[start + 1] == 0x40)) ++start;
    if (start == 31 && buffer[31] != 0x20) {
        size = 0;
    } else {
        size = 32 - start;
        for (int i = 0; i < size; ++i) buffer[i] = buffer[start + i];
    }
    return false;
}
