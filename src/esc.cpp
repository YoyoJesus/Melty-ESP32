#include "esc.h"
#include <Arduino.h>
static uint32_t frequency;
void escWrite(MotorPulses p) {
    ledcWrite(25, static_cast<uint32_t>((static_cast<uint64_t>(p.aUs) * frequency * 65536) / 1000000));
    ledcWrite(26, static_cast<uint32_t>((static_cast<uint64_t>(p.bUs) * frequency * 65536) / 1000000));
}
void escBegin(uint32_t hz) {
    frequency = hz;
    if (!ledcAttachChannel(25, hz, 16, 0) || !ledcAttachChannel(26, hz, 16, 1)) abort();
    escWrite(kNeutral);
}
