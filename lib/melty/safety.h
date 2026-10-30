#pragma once

#include <stdint.h>
#include "ibus.h"

constexpr uint16_t kChMin = 900, kChMax = 2100, kArmHigh = 1700, kThrottleLow = 1050, kFullLeft = 1050;
constexpr uint32_t kIbusTimeoutMs = 100;

class Safety {
public:
    void setSensorStartupOk(bool ok);
    void onFrame(const IbusFrame& f, uint32_t nowMs);
    void update(uint32_t nowMs);
    bool armed() const;
    bool signalOk() const;

private:
    bool sensorOk_ = false;
    bool armed_ = false;
    bool signalOk_ = false;
    bool disarmedSeen_ = false;
    uint32_t lastFrameMs_ = 0;
};
