#include <cmath>
#include <stdint.h>
#include <unity.h>
#include "spin.h"

constexpr float kRadius = 0.01f;
constexpr float k600RpmG = 4.0257f;

void test_rpm_16g_10mm_is_near_1197() { TEST_ASSERT_FLOAT_WITHIN(5.0f, 1197.0f, rpmFromAccel(16.0f, kRadius)); }
void test_heading_full_turn() {
    HeadingTracker h;
    h.update(k600RpmG, kRadius, 0, 100000, 0);
    TEST_ASSERT_TRUE(h.headingRad() < 0.001f || h.headingRad() > 6.282f);
}
void test_heading_stays_wrapped() {
    HeadingTracker h;
    h.update(k600RpmG, kRadius, 0, 1234567, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.002f, 2.171f, h.headingRad());
    h.update(k600RpmG, kRadius, -100.0f, 1000000, 1);
    TEST_ASSERT_TRUE(h.headingRad() >= 0 && h.headingRad() < 6.283186f);
}
void test_trim_shifts_heading() {
    HeadingTracker h;
    h.update(k600RpmG, kRadius, 1.0f, 100000, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, h.headingRad());
}
void test_clipped_reading_holds_rpm() {
    HeadingTracker h;
    h.update(k600RpmG, kRadius, 0, 0, 10);
    h.update(15.6f, kRadius, 0, 25000, 20);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 600.0f, h.rpm());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5708f, h.headingRad());
}
void test_nan_and_negative_hold_rpm() {
    HeadingTracker h;
    TEST_ASSERT_EQUAL_FLOAT(0, h.rpm());
    TEST_ASSERT_FALSE(h.sensorOk(0));
    h.update(NAN, kRadius, 0, 1000, 0);
    TEST_ASSERT_FALSE(h.sensorOk(0));
    h.update(k600RpmG, kRadius, 0, 0, 1);
    h.update(NAN, kRadius, 0, 10000, 2);
    h.update(-1, kRadius, 0, 10000, 3);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 600.0f, h.rpm());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.25664f, h.headingRad());
    TEST_ASSERT_TRUE(std::isfinite(h.headingRad()));
}
void test_recovers_on_next_good_reading() {
    HeadingTracker h;
    h.update(k600RpmG, kRadius, 0, 0, 0);
    h.update(15.6f, kRadius, 0, 0, 500);
    TEST_ASSERT_FALSE(h.sensorOk(500));
    h.update(1.0f, kRadius, 0, 0, 501);
    TEST_ASSERT_TRUE(h.sensorOk(501));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 299.04f, h.rpm());
}
void test_sensor_ok_false_at_500ms() {
    HeadingTracker h;
    h.update(1, kRadius, 0, 0, UINT32_MAX - 100);
    TEST_ASSERT_TRUE(h.sensorOk(398));
    TEST_ASSERT_FALSE(h.sensorOk(399));
}
void test_flip_ignores_short_impact() {
    FlipDetector f;
    f.update(1, 1, 1000, 1000);
    f.update(-1, 1, 1000, 50);
    TEST_ASSERT_FALSE(f.flipped());
}
void test_flip_after_sustained_inversion() {
    FlipDetector f;
    f.update(-1, -1, 1000, 1000);
    for (int i = 0; i < 200; ++i) f.update(1, -1, 1000, 10);
    TEST_ASSERT_TRUE(f.flipped());
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_rpm_16g_10mm_is_near_1197); RUN_TEST(test_heading_full_turn);
    RUN_TEST(test_heading_stays_wrapped); RUN_TEST(test_trim_shifts_heading);
    RUN_TEST(test_clipped_reading_holds_rpm); RUN_TEST(test_nan_and_negative_hold_rpm);
    RUN_TEST(test_recovers_on_next_good_reading); RUN_TEST(test_sensor_ok_false_at_500ms);
    RUN_TEST(test_flip_ignores_short_impact); RUN_TEST(test_flip_after_sustained_inversion);
    return UNITY_END();
}
