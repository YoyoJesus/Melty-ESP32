#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <initializer_list>
constexpr int OUTPUT = 1, LOW = 0;
constexpr int RMT_TX_MODE = 1, RMT_MEM_NUM_BLOCKS_1 = 1;
union rmt_data_t {
    struct { uint32_t duration0 : 15, level0 : 1, duration1 : 15, level1 : 1; };
    uint32_t val;
};
void pinMode(int pin, int mode);
void digitalWrite(int pin, int level);
bool rmtInit(int pin, int direction, int blocks, uint32_t frequency);
bool rmtSetEOT(int pin, uint8_t level);
bool rmtWriteAsync(int pin, rmt_data_t* data, size_t count);
