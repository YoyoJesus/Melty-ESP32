#pragma once

#include <stdint.h>

struct MotorPulses { uint16_t aUs, bUs; };
constexpr MotorPulses kNeutral{1500, 1500};

float rampStep(float current, float target, float ratePerSec, uint32_t dtMs);

class JamGuard {
public:
    float apply(float power, float rpm, float capPower, float minRpm, uint32_t dtMs);

private:
    uint32_t jamMs_ = 0;
    bool tripped_ = false;
};

struct DriveInput { float spin, x, y, headingRad, rpm; bool translationAllowed; };
struct DriveTuning { float minRpm, strength, phaseRad; bool reverseA, reverseB; };
MotorPulses computePulses(const DriveInput& in, const DriveTuning& t);
bool ledOn(float headingRad, float arcRad, bool sensorOk);
float effectiveHeading(float headingRad, bool flipped);
float stick(uint16_t ch);
float spinCommand(uint16_t ch3);
inline float mirrored(float value, bool flipped) { return flipped ? -value : value; }
