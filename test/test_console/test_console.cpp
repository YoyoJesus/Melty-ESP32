#include <cmath>
#include <cstring>
#include <string>
#include <unity.h>
#include "settings.h"
#include "console.h"
#include "runlog.h"

void test_defaults_valid() { Settings s; TEST_ASSERT_TRUE(settingsValid(s)); }
void test_all_fields_set() {
    Settings s;
    const char* names[] = {"radiusM", "trimRadPerSec", "rampPerSec", "minRpm", "stallCap", "strength", "phaseRad", "ledArcRad", "flipFilterMs", "outputHz", "radialAxis", "radialSign", "zSign", "reverseA", "reverseB"};
    const char* values[] = {"0.02", "4", "1", "300", "0.5", "0.6", "-1", "1", "500", "100", "1", "-1", "-1", "1", "1"};
    for (int i = 0; i < 15; ++i) TEST_ASSERT_TRUE_MESSAGE(setByName(s, names[i], values[i]), names[i]);
    TEST_ASSERT_EQUAL_FLOAT(0.02f, s.radiusM); TEST_ASSERT_EQUAL_INT(-1, s.zSign); TEST_ASSERT_TRUE(s.reverseB);
}
void test_invalid_values_leave_settings_unchanged() {
    Settings s;
    const char* bad[] = {"garbage", "nan", "inf", "0.2", "0.01x", "", " 0.01"};
    for (auto v : bad) { TEST_ASSERT_FALSE(setByName(s, "radiusM", v)); TEST_ASSERT_EQUAL_FLOAT(0.01f, s.radiusM); }
    TEST_ASSERT_FALSE(setByName(s, "unknown", "1"));
    TEST_ASSERT_FALSE(setByName(s, "reverseA", "2")); TEST_ASSERT_FALSE(s.reverseA);
    TEST_ASSERT_FALSE(setByName(s, "outputHz", "100x")); TEST_ASSERT_EQUAL_UINT32(50, s.outputHz);
}
void test_corrupt_settings() {
    Settings s; s.radiusM = 0; TEST_ASSERT_FALSE(settingsValid(s));
    s = Settings{}; s.outputHz = 0; TEST_ASSERT_FALSE(settingsValid(s));
    s = Settings{}; s.phaseRad = NAN; TEST_ASSERT_FALSE(settingsValid(s));
    s = Settings{}; s.flipFilterMs = 99; TEST_ASSERT_FALSE(settingsValid(s));
}
void test_console_commands() {
    Settings s;
    const char* lines[] = {"show", "set radiusM 0.02", "save", "defaults", "log", "status", "anything"};
    for (auto line : lines) TEST_ASSERT_TRUE(handleLine(line, true, s) == ConsoleResult::Rejected);
    TEST_ASSERT_EQUAL_FLOAT(0.01f, s.radiusM);
    TEST_ASSERT_TRUE(handleLine("show", false, s) == ConsoleResult::Ok);
    TEST_ASSERT_TRUE(handleLine("set radiusM 0.02", false, s) == ConsoleResult::Ok);
    TEST_ASSERT_EQUAL_FLOAT(0.02f, s.radiusM);
    TEST_ASSERT_TRUE(handleLine("save", false, s) == ConsoleResult::Save);
    TEST_ASSERT_TRUE(handleLine("defaults", false, s) == ConsoleResult::Defaults);
    TEST_ASSERT_EQUAL_FLOAT(0.02f, s.radiusM);
    TEST_ASSERT_TRUE(handleLine("log", false, s) == ConsoleResult::DumpLog);
    TEST_ASSERT_TRUE(handleLine("status", false, s) == ConsoleResult::ToggleStatus);
    TEST_ASSERT_TRUE(handleLine("", false, s) == ConsoleResult::Rejected);
    TEST_ASSERT_TRUE(handleLine(std::string(80, 'x').c_str(), false, s) == ConsoleResult::Rejected);
    TEST_ASSERT_TRUE(handleLine("show extra", false, s) == ConsoleResult::Rejected);
}
void test_format_settings() {
    Settings s; char out[512]; size_t n = formatSettings(s, out, sizeof(out));
    TEST_ASSERT_EQUAL(strlen(out), n); TEST_ASSERT_NOT_NULL(strstr(out, "radiusM")); TEST_ASSERT_NOT_NULL(strstr(out, "reverseB"));
    char tiny[4]; TEST_ASSERT_TRUE(formatSettings(s, tiny, sizeof(tiny)) >= sizeof(tiny)); TEST_ASSERT_EQUAL_CHAR('\0', tiny[3]);
}
void test_runlog_wraps() {
    RunLog<2> log; log.add({1, 1, 1, 1000, 1000}); log.add({2, 2, 2, 1000, 1000}); log.add({3, 3, 3, 1000, 1000});
    TEST_ASSERT_EQUAL_UINT(2, log.size()); TEST_ASSERT_EQUAL_UINT32(2, log.at(0).ms); TEST_ASSERT_EQUAL_UINT32(3, log.at(1).ms);
    log.clear(); TEST_ASSERT_EQUAL_UINT(0, log.size());
}
int main() { UNITY_BEGIN(); RUN_TEST(test_defaults_valid); RUN_TEST(test_all_fields_set); RUN_TEST(test_invalid_values_leave_settings_unchanged); RUN_TEST(test_corrupt_settings); RUN_TEST(test_console_commands); RUN_TEST(test_format_settings); RUN_TEST(test_runlog_wraps); return UNITY_END(); }
