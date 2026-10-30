#include "drive.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2 * kPi;

uint16_t pulse(float power, bool reverse) {
    const int offset = static_cast<int>(std::lround(500 * power));
    return static_cast<uint16_t>(1500 + (reverse ? -offset : offset));
}
}

float rampStep(float current, float target, float ratePerSec, uint32_t dtMs) {
    return target <= current ? target : std::min(target, current + ratePerSec * (dtMs / 1000.0f));
}

float JamGuard::apply(float power, float rpm, float capPower, float minRpm, uint32_t dtMs) {
    if (rpm >= minRpm) tripped_ = false;
    if (power > capPower && rpm < minRpm) {
        jamMs_ += std::min(dtMs, 1000u - jamMs_);
        if (jamMs_ >= 1000) tripped_ = true;
    } else {
        jamMs_ = 0;
    }
    return tripped_ ? std::min(power, capPower) : power;
}

MotorPulses computePulses(const DriveInput& in, const DriveTuning& t) {
    const float mag = std::min(1.0f, std::hypot(in.x, in.y));
    const float dir = std::atan2(in.x, in.y);
    const float sign = std::cos(in.headingRad - dir - t.phaseRad) >= 0 ? 1.0f : -1.0f;
    const float d = in.translationAllowed && in.rpm >= t.minRpm && mag > 0 ? t.strength * mag * sign : 0;
    const float spin = std::clamp(in.spin, 0.0f, 1.0f);
    return {pulse(std::clamp(spin + d, 0.0f, 1.0f), t.reverseA),
            pulse(std::clamp(spin - d, 0.0f, 1.0f), t.reverseB)};
}

float effectiveHeading(float headingRad, bool flipped) {
    float heading = std::fmod(flipped ? -headingRad : headingRad, kTwoPi);
    if (heading < 0) heading += kTwoPi;
    if (heading >= kTwoPi) heading = 0;
    return heading;
}

bool ledOn(float headingRad, float arcRad, bool sensorOk) {
    const float heading = effectiveHeading(headingRad, false);
    return sensorOk && std::min(heading, kTwoPi - heading) <= arcRad / 2;
}

float stick(uint16_t ch) {
    const float value = std::clamp((static_cast<float>(ch) - 1500) / 500, -1.0f, 1.0f);
    return std::abs(value) <= 0.05f ? 0 : value;
}

float spinCommand(uint16_t ch3) {
    return std::clamp((static_cast<float>(ch3) - 1000) / 1000, 0.0f, 1.0f);
}
