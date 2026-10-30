#pragma once

#include <stdint.h>

constexpr float kClipG = 15.5f, kGravity = 9.80665f;
float rpmFromAccel(float radialG, float radiusM);

class HeadingTracker {
public:
    void update(float radialG, float radiusM, float trimRadPerSec, uint32_t dtUs, uint32_t nowMs);
    float headingRad() const;
    float rpm() const;
    bool sensorOk(uint32_t nowMs) const;

private:
    float headingRad_ = 0;
    float rpm_ = 0;
    uint32_t lastGoodMs_ = 0;
    bool hasGoodReading_ = false;
};

class FlipDetector {
public:
    void update(float zG, int zSign, uint32_t filterMs, uint32_t dtMs);
    bool flipped() const;

private:
    float filteredZ_ = 0;
};
