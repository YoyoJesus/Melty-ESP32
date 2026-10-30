#include <cmath>
#include <unity.h>
#include "drive.h"

constexpr float pi = 3.14159265358979323846f;
const DriveTuning tuning{100, 0.2f, 0, false, false};

void test_ramp_limits_rise_and_drops_instantly() {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.3f, rampStep(0.2f, 0.8f, 0.4f, 250));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.25f, rampStep(0.2f, 0.25f, 0.4f, 250));
    TEST_ASSERT_EQUAL_FLOAT(0.1f, rampStep(0.8f, 0.1f, 0.4f, 250));
}
void test_zero_spin_is_exact_neutral() {
    auto p = computePulses({0, 0, 1, 0, 200, true}, tuning);
    TEST_ASSERT_EQUAL_UINT16(1500, p.aUs); TEST_ASSERT_EQUAL_UINT16(1500, p.bUs);
}
void test_low_spin_limits_translation() {
    auto p = computePulses({0.1f, 0, 1, 0, 200, true}, tuning);
    TEST_ASSERT_TRUE(p.aUs <= 1600); TEST_ASSERT_TRUE(p.bUs <= 1600);
}
void test_pulses_never_cross_neutral() {
    for (int h = 0; h < 36; ++h) for (int s = -1; s <= 11; ++s) for (int x = -1; x <= 1; ++x) {
        auto p = computePulses({s / 10.0f, static_cast<float>(x), 1, h * pi / 18, 200, true}, tuning);
        TEST_ASSERT_TRUE(p.aUs >= 1500 && p.aUs <= 2000);
        TEST_ASSERT_TRUE(p.bUs >= 1500 && p.bUs <= 2000);
    }
}
void test_reverse_mirrors_around_1500() {
    DriveInput in{0.5f, 0, 1, 0, 200, true};
    auto p = computePulses(in, tuning);
    auto q = computePulses(in, {100, 0.2f, 0, true, true});
    TEST_ASSERT_EQUAL_INT(3000, p.aUs + q.aUs); TEST_ASSERT_EQUAL_INT(3000, p.bUs + q.bUs);
}
void test_no_translation_below_min_rpm() {
    auto p = computePulses({0.5f, 0, 1, 0, 99, true}, tuning);
    TEST_ASSERT_EQUAL_UINT16(1750, p.aUs); TEST_ASSERT_EQUAL_UINT16(1750, p.bUs);
}
void test_no_translation_when_disallowed() {
    auto p = computePulses({0.5f, 0, 1, 0, 200, false}, tuning);
    TEST_ASSERT_EQUAL_UINT16(1750, p.aUs); TEST_ASSERT_EQUAL_UINT16(1750, p.bUs);
}
void test_centered_stick_no_translation() {
    for (int h = 0; h < 36; ++h) {
        auto p = computePulses({0.5f, 0, 0, h * pi / 18, 200, true}, tuning);
        TEST_ASSERT_EQUAL_UINT16(1750, p.aUs); TEST_ASSERT_EQUAL_UINT16(1750, p.bUs);
    }
}
void test_translation_opposes_half_turn_apart() {
    auto a = computePulses({0.5f, 0, 1, 0, 100, true}, tuning);
    auto b = computePulses({0.5f, 0, 1, pi, 100, true}, tuning);
    TEST_ASSERT_EQUAL_UINT16(1850, a.aUs); TEST_ASSERT_EQUAL_UINT16(1650, a.bUs);
    TEST_ASSERT_EQUAL_UINT16(a.aUs, b.bUs); TEST_ASSERT_EQUAL_UINT16(a.bUs, b.aUs);
    auto phase = computePulses({0.5f, 0, 1, 0, 200, true}, {100, 0.2f, pi, false, false});
    TEST_ASSERT_EQUAL_UINT16(b.aUs, phase.aUs);
}
void test_jam_caps_after_1000ms() {
    JamGuard j;
    TEST_ASSERT_EQUAL_FLOAT(0.8f, j.apply(0.8f, 0, 0.4f, 100, 999));
    TEST_ASSERT_EQUAL_FLOAT(0.4f, j.apply(0.8f, 0, 0.4f, 100, 1));
    TEST_ASSERT_EQUAL_FLOAT(0.3f, j.apply(0.3f, 0, 0.4f, 100, 1));
}
void test_jam_ignored_at_or_below_cap() {
    JamGuard j;
    TEST_ASSERT_EQUAL_FLOAT(0.4f, j.apply(0.4f, 0, 0.4f, 100, 1000));
    TEST_ASSERT_EQUAL_FLOAT(0.8f, j.apply(0.8f, 0, 0.4f, 100, 999));
    j.apply(0.3f, 0, 0.4f, 100, 1);
    TEST_ASSERT_EQUAL_FLOAT(0.8f, j.apply(0.8f, 0, 0.4f, 100, 999));
}
void test_jam_releases_when_rpm_recovers() {
    JamGuard j;
    j.apply(0.8f, 0, 0.4f, 100, 1000);
    TEST_ASSERT_EQUAL_FLOAT(0.8f, j.apply(0.8f, 100, 0.4f, 100, 1));
    TEST_ASSERT_EQUAL_FLOAT(0.8f, j.apply(0.8f, 0, 0.4f, 100, 999));
}
void test_led_on_only_near_forward() {
    TEST_ASSERT_TRUE(ledOn(0, 1, true)); TEST_ASSERT_TRUE(ledOn(0.5f, 1, true));
    TEST_ASSERT_TRUE(ledOn(2 * pi - 0.4f, 1, true)); TEST_ASSERT_TRUE(ledOn(-0.4f, 1, true));
    TEST_ASSERT_FALSE(ledOn(0.6f, 1, true)); TEST_ASSERT_FALSE(ledOn(pi, 1, true));
}
void test_led_off_when_sensor_bad() { TEST_ASSERT_FALSE(ledOn(0, 1, false)); }
void test_effective_heading_mirrors_when_flipped() {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1, effectiveHeading(1, false));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2 * pi - 1, effectiveHeading(1, true));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1, effectiveHeading(-1, true));
}
void test_stick_deadband_and_limits() {
    TEST_ASSERT_EQUAL_FLOAT(0, stick(1500)); TEST_ASSERT_EQUAL_FLOAT(0, stick(1525));
    TEST_ASSERT_EQUAL_FLOAT(0, stick(1475)); TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.052f, stick(1526));
    TEST_ASSERT_EQUAL_FLOAT(-1, stick(900)); TEST_ASSERT_EQUAL_FLOAT(1, stick(2100));
    TEST_ASSERT_EQUAL_FLOAT(0, spinCommand(900)); TEST_ASSERT_EQUAL_FLOAT(1, spinCommand(2100));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, spinCommand(1500));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_ramp_limits_rise_and_drops_instantly); RUN_TEST(test_zero_spin_is_exact_neutral);
    RUN_TEST(test_low_spin_limits_translation); RUN_TEST(test_pulses_never_cross_neutral); RUN_TEST(test_reverse_mirrors_around_1500);
    RUN_TEST(test_no_translation_below_min_rpm); RUN_TEST(test_no_translation_when_disallowed);
    RUN_TEST(test_centered_stick_no_translation); RUN_TEST(test_translation_opposes_half_turn_apart);
    RUN_TEST(test_jam_caps_after_1000ms); RUN_TEST(test_jam_ignored_at_or_below_cap);
    RUN_TEST(test_jam_releases_when_rpm_recovers); RUN_TEST(test_led_on_only_near_forward);
    RUN_TEST(test_led_off_when_sensor_bad); RUN_TEST(test_effective_heading_mirrors_when_flipped);
    RUN_TEST(test_stick_deadband_and_limits);
    return UNITY_END();
}
