/**
 * test_modbus.cpp — Unit tests for regsToFloat()
 * Run with: pio test -e native
 */
#include <unity.h>
#include <stdint.h>
#include <string.h>

// Inline copy of regsToFloat for native testing (no Arduino hardware)
static float regsToFloat(uint16_t hi, uint16_t lo) {
    uint32_t v = ((uint32_t)hi << 16) | lo;
    float f;
    memcpy(&f, &v, sizeof(f));
    return f;
}

void test_regsToFloat_250V() {
    // 250.0f = 0x437A0000 → hi=0x437A lo=0x0000
    float result = regsToFloat(0x437A, 0x0000);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 250.0f, result);
}

void test_regsToFloat_zero() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, regsToFloat(0x0000, 0x0000));
}

void test_regsToFloat_negative_pf() {
    // -1.0f = 0xBF800000 → hi=0xBF80 lo=0x0000
    float result = regsToFloat(0xBF80, 0x0000);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, result);
}

void test_regsToFloat_50Hz() {
    // 50.0f = 0x42480000 → hi=0x4248 lo=0x0000
    float result = regsToFloat(0x4248, 0x0000);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, result);
}

void test_regsToFloat_large_energy() {
    // 0x461C3C00 = 9999.0f exactly:
    //   exponent=13 (biased 140), mantissa=1850368
    //   value = (1 + 1807/8192) × 8192 = 8192 + 1807 = 9999.0
    // Validates large-value reconstruction and correct byte ordering.
    float result = regsToFloat(0x461C, 0x3C00);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 9999.0f, result);
}

void test_regsToFloat_230V_typical() {
    // 230.0f = 0x43660000 — typical phase voltage
    float result = regsToFloat(0x4366, 0x0000);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 230.0f, result);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_regsToFloat_250V);
    RUN_TEST(test_regsToFloat_zero);
    RUN_TEST(test_regsToFloat_negative_pf);
    RUN_TEST(test_regsToFloat_50Hz);
    RUN_TEST(test_regsToFloat_large_energy);
    RUN_TEST(test_regsToFloat_230V_typical);
    return UNITY_END();
}
