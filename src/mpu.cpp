#include "mpu.h"
#include <Arduino.h>
#include <Wire.h>
#include <cmath>
static bool writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(0x68); Wire.write(reg); Wire.write(value); return Wire.endTransmission() == 0;
}
bool mpuBegin() {
    Wire.begin(21, 22, 400000); Wire.setTimeOut(2);
    return writeReg(0x6b, 0) && writeReg(0x1c, 0x18);
}
bool mpuRead(float& xG, float& yG, float& zG) {
    Wire.beginTransmission(0x68); Wire.write(0x3b);
    if (Wire.endTransmission(false) != 0 || Wire.requestFrom(0x68, 6) != 6) return false;
    int16_t v[3];
    for (int i = 0; i < 3; ++i) { uint16_t hi = Wire.read(); v[i] = static_cast<int16_t>((hi << 8) | Wire.read()); }
    xG = v[0] / 2048.0f; yG = v[1] / 2048.0f; zG = v[2] / 2048.0f; return true;
}
bool mpuRestCheck() {
    for (int i = 0; i < 3; ++i) {
        float x, y, z;
        if (!mpuRead(x, y, z)) return false;
        float magnitude = sqrtf(x*x + y*y + z*z);
        if (magnitude < 0.7f || magnitude > 1.3f) return false;
    }
    return true;
}
