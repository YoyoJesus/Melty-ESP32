#include "safety.h"

void Safety::setSensorStartupOk(bool ok) { sensorOk_ = ok; }

void Safety::onFrame(const IbusFrame& f, uint32_t nowMs) {
    update(nowMs);
    bool valid = !f.failsafeFlag;
    for (int i = 0; i < 5; ++i) valid = valid && f.ch[i] >= kChMin && f.ch[i] <= kChMax;
    if (!valid) {
        armed_ = false;
        signalOk_ = false;
        disarmedSeen_ = false;
        return;
    }
    signalOk_ = true;
    lastFrameMs_ = nowMs;
    if (f.ch[4] <= kArmHigh) {
        armed_ = false;
        disarmedSeen_ = true;
    } else if (disarmedSeen_) {
        armed_ = sensorOk_ && f.ch[2] <= kThrottleLow && f.ch[0] <= kFullLeft;
        disarmedSeen_ = false;
    }
}

void Safety::update(uint32_t nowMs) {
    if (signalOk_ && nowMs - lastFrameMs_ >= kIbusTimeoutMs) {
        signalOk_ = false;
        armed_ = false;
        disarmedSeen_ = false;
    }
}

bool Safety::armed() const { return armed_; }
bool Safety::signalOk() const { return signalOk_; }
