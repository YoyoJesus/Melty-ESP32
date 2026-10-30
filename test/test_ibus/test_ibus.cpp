#include <stdint.h>
#include <unity.h>
#include "ibus.h"

static void make_frame(uint8_t (&frame)[32], const uint16_t (&channels)[14]) {
    frame[0] = 0x20;
    frame[1] = 0x40;
    uint16_t checksum = 0xffff - frame[0] - frame[1];
    for (int i = 0; i < 14; ++i) {
        frame[2 + i * 2] = channels[i] & 0xff;
        frame[3 + i * 2] = channels[i] >> 8;
        checksum -= frame[2 + i * 2] + frame[3 + i * 2];
    }
    frame[30] = checksum & 0xff;
    frame[31] = checksum >> 8;
}

static bool feed(IbusParser& parser, IbusFrame& out, const uint8_t* bytes, int count) {
    bool decoded = false;
    for (int i = 0; i < count; ++i) decoded |= parser.push(bytes[i], out);
    return decoded;
}

void test_valid_frame_decodes() {
    uint16_t channels[14] = {1500, 1200, 1300, 1400, 1000};
    uint8_t frame[32]; make_frame(frame, channels);
    IbusParser parser; IbusFrame out{};
    for (int i = 0; i < 31; ++i) TEST_ASSERT_FALSE(parser.push(frame[i], out));
    TEST_ASSERT_TRUE(parser.push(frame[31], out));
    TEST_ASSERT_EQUAL_UINT16(1500, out.ch[0]);
    TEST_ASSERT_EQUAL_UINT16(1000, out.ch[4]);
    TEST_ASSERT_FALSE(out.failsafeFlag);
}

void test_bad_checksum_rejected() {
    uint16_t channels[14] = {}; uint8_t frame[32]; make_frame(frame, channels);
    frame[30] ^= 1;
    IbusParser parser; IbusFrame out{};
    TEST_ASSERT_FALSE(feed(parser, out, frame, 32));
}

void test_resync_after_junk_bytes() {
    uint16_t channels[14] = {1500}; uint8_t frame[32]; make_frame(frame, channels);
    const uint8_t junk[] = {0x10, 0x20, 0x12, 0x20};
    IbusParser parser; IbusFrame out{};
    TEST_ASSERT_FALSE(feed(parser, out, junk, sizeof(junk)));
    TEST_ASSERT_TRUE(feed(parser, out, frame, 32));
    TEST_ASSERT_EQUAL_UINT16(1500, out.ch[0]);
}

void test_truncated_frame_then_good() {
    uint16_t channels[14] = {1500}; uint8_t frame[32]; make_frame(frame, channels);
    IbusParser parser; IbusFrame out{};
    TEST_ASSERT_FALSE(feed(parser, out, frame, 10));
    TEST_ASSERT_TRUE(feed(parser, out, frame, 32));
    TEST_ASSERT_EQUAL_UINT16(1500, out.ch[0]);
}

void test_failsafe_flag_on_used_channel() {
    uint16_t channels[14] = {0, 0, 0x2000 | 1000}; uint8_t frame[32]; make_frame(frame, channels);
    IbusParser parser; IbusFrame out{};
    TEST_ASSERT_TRUE(feed(parser, out, frame, 32));
    TEST_ASSERT_TRUE(out.failsafeFlag);
    TEST_ASSERT_EQUAL_UINT16(1000, out.ch[2]);
}

void test_upper_bits_on_ch7_ignored() {
    uint16_t channels[14] = {0, 0, 0, 0, 0, 0, 0x2000 | 1000}; uint8_t frame[32]; make_frame(frame, channels);
    IbusParser parser; IbusFrame out{};
    TEST_ASSERT_TRUE(feed(parser, out, frame, 32));
    TEST_ASSERT_FALSE(out.failsafeFlag);
    TEST_ASSERT_EQUAL_UINT16(1000, out.ch[6]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_valid_frame_decodes);
    RUN_TEST(test_bad_checksum_rejected);
    RUN_TEST(test_resync_after_junk_bytes);
    RUN_TEST(test_truncated_frame_then_good);
    RUN_TEST(test_failsafe_flag_on_used_channel);
    RUN_TEST(test_upper_bits_on_ch7_ignored);
    return UNITY_END();
}
