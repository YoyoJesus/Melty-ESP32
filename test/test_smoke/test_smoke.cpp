#include <unity.h>

void test_smoke() {
    TEST_ASSERT_EQUAL(4, 2 + 2);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_smoke);
    return UNITY_END();
}
