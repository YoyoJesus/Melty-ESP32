#include "spin.h"
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
}

float rpmFromAccel(float radialG, float radiusM) {
    return std::sqrt(radialG * kGravity / radiusM) * 60.0f / kTwoPi;
}

void HeadingTracker::update(float radialG, float radiusM, float trimRadPerSec, uint32_t dtUs, uint32_t nowMs) {
    if (std::isfinite(radialG) && radialG > 0 && radialG < kClipG) {
        rpm_ = rpmFromAccel(radialG, radiusM);
        lastGoodMs_ = nowMs;
        hasGoodReading_ = true;
    }
    headingRad_ = std::fmod(headingRad_ + (rpm_ * kTwoPi / 60.0f + trimRadPerSec) * (dtUs / 1000000.0f), kTwoPi);
    if (headingRad_ < 0) headingRad_ += kTwoPi;
    if (headingRad_ >= kTwoPi) headingRad_ = 0;  // a tiny negative remainder rounds up to exactly 2pi
}

float HeadingTracker::headingRad() const { return headingRad_; }
float HeadingTracker::rpm() const { return rpm_; }
bool HeadingTracker::sensorOk(uint32_t nowMs) const { return hasGoodReading_ && nowMs - lastGoodMs_ < 500; }

void FlipDetector::update(float zG, int zSign, uint32_t filterMs, uint32_t dtMs) {
    const float alpha = filterMs == 0 ? 1.0f : static_cast<float>(dtMs) / (static_cast<float>(filterMs) + dtMs);
    filteredZ_ += alpha * (zG * zSign - filteredZ_);
}

bool FlipDetector::flipped() const { return filteredZ_ < 0; }
