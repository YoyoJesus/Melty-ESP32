#pragma once
#include <stddef.h>
#include <stdint.h>

struct Settings {
    float radiusM = 0.010f, trimRadPerSec = 3.14f, rampPerSec = 0.25f, minRpm = 200.0f,
          stallCap = 0.3f, strength = 0.25f, phaseRad = 0.0f, ledArcRad = 0.5f;
    uint32_t flipFilterMs = 1000, outputHz = 50;
    int radialAxis = 0, radialSign = 1, zSign = 1;
    bool reverseA = false, reverseB = false;
};
bool settingsValid(const Settings& s);
bool setByName(Settings& s, const char* name, const char* value);
size_t formatSettings(const Settings& s, char* out, size_t outLen);
