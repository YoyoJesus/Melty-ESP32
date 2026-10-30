#include "settings.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

static bool between(float v, float lo, float hi) { return std::isfinite(v) && v >= lo && v <= hi; }
bool settingsValid(const Settings& s) {
    return between(s.radiusM, 0.002f, 0.1f) && between(s.trimRadPerSec, 0, 20) &&
        between(s.rampPerSec, 0.05f, 5) && between(s.minRpm, 50, 2000) &&
        between(s.stallCap, 0, 1) && between(s.strength, 0, 1) &&
        between(s.phaseRad, -6.3f, 6.3f) && between(s.ledArcRad, 0.05f, 3.2f) &&
        s.flipFilterMs >= 100 && s.flipFilterMs <= 10000 && s.outputHz >= 50 && s.outputHz <= 400 &&
        s.radialAxis >= 0 && s.radialAxis <= 1 && (s.radialSign == -1 || s.radialSign == 1) &&
        (s.zSign == -1 || s.zSign == 1);
}
bool setByName(Settings& s, const char* name, const char* value) {
    if (!name || !value || !*value || std::isspace(static_cast<unsigned char>(*value))) return false;
    Settings copy = s;
    float* f = nullptr;
    if (!strcmp(name, "radiusM")) f = &copy.radiusM;
    else if (!strcmp(name, "trimRadPerSec")) f = &copy.trimRadPerSec;
    else if (!strcmp(name, "rampPerSec")) f = &copy.rampPerSec;
    else if (!strcmp(name, "minRpm")) f = &copy.minRpm;
    else if (!strcmp(name, "stallCap")) f = &copy.stallCap;
    else if (!strcmp(name, "strength")) f = &copy.strength;
    else if (!strcmp(name, "phaseRad")) f = &copy.phaseRad;
    else if (!strcmp(name, "ledArcRad")) f = &copy.ledArcRad;
    char* end;
    errno = 0;
    if (f) {
        float v = strtof(value, &end);
        if (errno == ERANGE || end == value || *end || !std::isfinite(v)) return false;
        *f = v;
    } else {
        long v = strtol(value, &end, 10);
        if (errno == ERANGE || end == value || *end) return false;
        if (!strcmp(name, "flipFilterMs")) { if (v < 100 || v > 10000) return false; copy.flipFilterMs = static_cast<uint32_t>(v); }
        else if (!strcmp(name, "outputHz")) { if (v < 50 || v > 400) return false; copy.outputHz = static_cast<uint32_t>(v); }
        else if (!strcmp(name, "radialAxis")) { if (v < 0 || v > 1) return false; copy.radialAxis = static_cast<int>(v); }
        else if (!strcmp(name, "radialSign")) { if (v != -1 && v != 1) return false; copy.radialSign = static_cast<int>(v); }
        else if (!strcmp(name, "zSign")) { if (v != -1 && v != 1) return false; copy.zSign = static_cast<int>(v); }
        else if (!strcmp(name, "reverseA")) { if (strcmp(value, "0") && strcmp(value, "1")) return false; copy.reverseA = v; }
        else if (!strcmp(name, "reverseB")) { if (strcmp(value, "0") && strcmp(value, "1")) return false; copy.reverseB = v; }
        else return false;
    }
    if (!settingsValid(copy)) return false;
    s = copy; return true;
}
size_t formatSettings(const Settings& s, char* out, size_t outLen) {
    int n = snprintf(out, outLen, "radiusM=%g\ntrimRadPerSec=%g\nrampPerSec=%g\nminRpm=%g\nstallCap=%g\nstrength=%g\nphaseRad=%g\nledArcRad=%g\nflipFilterMs=%lu\noutputHz=%lu\nradialAxis=%d\nradialSign=%d\nzSign=%d\nreverseA=%d\nreverseB=%d\n", s.radiusM, s.trimRadPerSec, s.rampPerSec, s.minRpm, s.stallCap, s.strength, s.phaseRad, s.ledArcRad, static_cast<unsigned long>(s.flipFilterMs), static_cast<unsigned long>(s.outputHz), s.radialAxis, s.radialSign, s.zSign, s.reverseA, s.reverseB);
    return n < 0 ? 0 : static_cast<size_t>(n);
}
