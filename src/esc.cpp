#include "esc.h"
#include <Arduino.h>

static rmt_data_t symbols[2][2]{};
static unsigned nextBuffer[2]{};
static uint16_t wanted[2]{};
static bool sent[2]{};

void escBegin() {
    for (int pin : {25, 26}) {
        pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
        if (!rmtInit(pin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 1000000) || !rmtSetEOT(pin, LOW)) abort();
    }
}
bool escWrite(MotorPulses p) {
    const uint16_t widths[] = {p.aUs, p.bUs};
    bool accepted = true;
    for (unsigned ch = 0; ch < 2; ++ch) {
        if (wanted[ch] != widths[ch]) { wanted[ch] = widths[ch]; sent[ch] = false; }
        if (sent[ch]) continue;
        // Keep the in-flight buffer unchanged until RMT completes it.
        rmt_data_t& symbol = symbols[ch][nextBuffer[ch]];
        symbol.val = 0;
        symbol.level0 = 1; symbol.duration0 = widths[ch];
        symbol.level1 = 0; symbol.duration1 = 1;
        if (rmtWriteAsync(25 + ch, &symbol, 1)) { nextBuffer[ch] ^= 1; sent[ch] = true; }
        else accepted = false;
    }
    if (accepted) sent[0] = sent[1] = false;
    return accepted;
}
