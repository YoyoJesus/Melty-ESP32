#include <unity.h>
#include <Arduino.h>
#include "../../src/esc.cpp"

static bool busy[2]{};
static rmt_data_t* active[2]{};
static unsigned writes[2]{}, initCount;
void pinMode(int pin, int mode) { TEST_ASSERT_TRUE(pin == 25 || pin == 26); TEST_ASSERT_EQUAL(OUTPUT, mode); }
void digitalWrite(int, int level) { TEST_ASSERT_EQUAL(LOW, level); }
bool rmtInit(int, int direction, int blocks, uint32_t frequency) {
    TEST_ASSERT_EQUAL(RMT_TX_MODE, direction); TEST_ASSERT_EQUAL(1, blocks);
    TEST_ASSERT_EQUAL_UINT32(1000000, frequency); ++initCount; return true;
}
bool rmtSetEOT(int, uint8_t level) { TEST_ASSERT_EQUAL(LOW, level); return true; }
bool rmtWriteAsync(int pin, rmt_data_t* data, size_t count) {
    const int ch = pin - 25;
    TEST_ASSERT_EQUAL(1, count);
    if (busy[ch]) return false;
    active[ch] = data; busy[ch] = true; ++writes[ch]; return true;
}
void test_one_shots_retry_each_channel_with_latest_neutral() {
    escBegin();
    TEST_ASSERT_EQUAL(2, initCount);
    TEST_ASSERT_EQUAL(0, writes[0] + writes[1]);
    TEST_ASSERT_TRUE(escWrite({1900, 1800}));
    TEST_ASSERT_EQUAL(1900, active[0]->duration0); TEST_ASSERT_EQUAL(1800, active[1]->duration0);
    TEST_ASSERT_EQUAL(1, active[0]->level0); TEST_ASSERT_EQUAL(0, active[0]->level1);
    TEST_ASSERT_FALSE(escWrite({1700, 1600}));
    TEST_ASSERT_EQUAL(1900, active[0]->duration0); TEST_ASSERT_EQUAL(1800, active[1]->duration0);
    busy[0] = false;
    TEST_ASSERT_FALSE(escWrite(kNeutral));
    TEST_ASSERT_EQUAL(1500, active[0]->duration0); TEST_ASSERT_EQUAL(1800, active[1]->duration0);
    busy[1] = false;
    TEST_ASSERT_TRUE(escWrite(kNeutral));
    TEST_ASSERT_EQUAL(1500, active[0]->duration0); TEST_ASSERT_EQUAL(1500, active[1]->duration0);
    TEST_ASSERT_EQUAL(2, writes[0]); TEST_ASSERT_EQUAL(2, writes[1]);
    busy[0] = busy[1] = false;
    TEST_ASSERT_TRUE(escWrite(kNeutral));
    TEST_ASSERT_EQUAL(3, writes[0]); TEST_ASSERT_EQUAL(3, writes[1]);
}
int main() {
    UNITY_BEGIN(); RUN_TEST(test_one_shots_retry_each_channel_with_latest_neutral); return UNITY_END();
}
