#include <stdint.h>
#include <initializer_list>
#include <unity.h>
#include "safety.h"

static IbusFrame frame(uint16_t arm = 1000, uint16_t x = 1000, uint16_t throttle = 1000) {
    IbusFrame f{};
    f.ch[0] = x; f.ch[1] = 1500; f.ch[2] = throttle; f.ch[3] = 1500; f.ch[4] = arm;
    return f;
}
static void arm(Safety& s, uint32_t t = 0) {
    s.setSensorStartupOk(true); s.onFrame(frame(), t); s.onFrame(frame(1800), t + 1);
}
void test_boot_disarmed() { Safety s; TEST_ASSERT_FALSE(s.armed()); TEST_ASSERT_FALSE(s.signalOk()); }
void test_arm_full_sequence() { Safety s; arm(s); TEST_ASSERT_TRUE(s.armed()); TEST_ASSERT_TRUE(s.signalOk()); }
void test_switch_armed_at_boot_never_arms() { Safety s; s.setSensorStartupOk(true); s.onFrame(frame(1800), 0); s.onFrame(frame(1800), 1); TEST_ASSERT_FALSE(s.armed()); }
void test_edge_with_stick_centered_does_not_arm_until_repeated() {
    Safety s; s.setSensorStartupOk(true); s.onFrame(frame(), 0); s.onFrame(frame(1800, 1500), 1);
    s.onFrame(frame(1800), 2); TEST_ASSERT_FALSE(s.armed()); s.onFrame(frame(), 3); s.onFrame(frame(1800), 4); TEST_ASSERT_TRUE(s.armed());
}
void test_edge_with_throttle_up_does_not_arm() { Safety s; s.setSensorStartupOk(true); s.onFrame(frame(), 0); s.onFrame(frame(1800, 1000, 1100), 1); TEST_ASSERT_FALSE(s.armed()); }
void test_edge_without_sensor_ok_does_not_arm() { Safety s; s.onFrame(frame(), 0); s.onFrame(frame(1800), 1); s.setSensorStartupOk(true); s.onFrame(frame(1800), 2); TEST_ASSERT_FALSE(s.armed()); }
void test_ch5_low_disarms() { Safety s; arm(s); s.onFrame(frame(1700), 2); TEST_ASSERT_FALSE(s.armed()); }
void test_timeout_at_100ms_disarms_and_99ms_does_not() { Safety s; arm(s); s.update(100); TEST_ASSERT_TRUE(s.armed()); s.update(101); TEST_ASSERT_FALSE(s.armed()); TEST_ASSERT_FALSE(s.signalOk()); }
void test_failsafe_flag_disarms() { Safety s; arm(s); IbusFrame f = frame(1800); f.failsafeFlag = true; s.onFrame(f, 2); TEST_ASSERT_FALSE(s.armed()); TEST_ASSERT_FALSE(s.signalOk()); }
void test_out_of_range_used_channel_disarms() {
    for (uint16_t value : {uint16_t(2200), uint16_t(0)}) { Safety s; arm(s); IbusFrame f = frame(1800); f.ch[1] = value; s.onFrame(f, 2); TEST_ASSERT_FALSE(s.armed()); TEST_ASSERT_FALSE(s.signalOk()); }
}
void test_failsafe_then_return_with_switch_armed_stays_disarmed() { Safety s; arm(s); s.onFrame(frame(1000, 1500), 2); s.onFrame(frame(1800, 1500), 3); TEST_ASSERT_FALSE(s.armed()); }
void test_timeout_then_frames_resume_armed_stays_disarmed() { Safety s; arm(s); s.update(101); s.onFrame(frame(1800), 102); TEST_ASSERT_FALSE(s.armed()); }
void test_time_wraparound() { Safety s; arm(s, UINT32_MAX - 50); s.update(49); TEST_ASSERT_TRUE(s.armed()); s.update(50); TEST_ASSERT_FALSE(s.armed()); }
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_boot_disarmed); RUN_TEST(test_arm_full_sequence); RUN_TEST(test_switch_armed_at_boot_never_arms);
    RUN_TEST(test_edge_with_stick_centered_does_not_arm_until_repeated); RUN_TEST(test_edge_with_throttle_up_does_not_arm);
    RUN_TEST(test_edge_without_sensor_ok_does_not_arm); RUN_TEST(test_ch5_low_disarms); RUN_TEST(test_timeout_at_100ms_disarms_and_99ms_does_not);
    RUN_TEST(test_failsafe_flag_disarms); RUN_TEST(test_out_of_range_used_channel_disarms); RUN_TEST(test_failsafe_then_return_with_switch_armed_stays_disarmed);
    RUN_TEST(test_timeout_then_frames_resume_armed_stays_disarmed); RUN_TEST(test_time_wraparound);
    return UNITY_END();
}
