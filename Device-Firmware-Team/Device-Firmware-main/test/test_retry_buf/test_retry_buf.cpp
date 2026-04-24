/**
 * test_retry_buf.cpp — Unit tests for MQTT retry ring buffer
 * Tests: FIFO order, overflow behaviour, partial flush
 * Run with: pio test -e native
 */
#include <unity.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define RETRY_BUF_SIZE    5
#define MQTT_PAYLOAD_MAX  640

// ── Inline buffer implementation (mirrors mqtt_manager.cpp) ───────────────
static char g_buf[RETRY_BUF_SIZE][MQTT_PAYLOAD_MAX];
static int  g_head  = 0;
static int  g_count = 0;
static bool g_overflowed = false;

static void bufReset() {
    g_head = g_count = 0;
    g_overflowed = false;
}

static void bufPush(const char* payload) {
    strncpy(g_buf[g_head], payload, MQTT_PAYLOAD_MAX - 1);
    g_buf[g_head][MQTT_PAYLOAD_MAX - 1] = '\0';
    g_head = (g_head + 1) % RETRY_BUF_SIZE;
    if (g_count < RETRY_BUF_SIZE) g_count++;
    else g_overflowed = true;
}

static const char* bufPeek() {
    if (g_count == 0) return nullptr;
    int tail = ((g_head - g_count) % RETRY_BUF_SIZE + RETRY_BUF_SIZE) % RETRY_BUF_SIZE;
    return g_buf[tail];
}

static void bufPop() {
    if (g_count > 0) g_count--;
}

// ── Tests ─────────────────────────────────────────────────────────────────
void test_push_and_fifo_order() {
    bufReset();
    bufPush("msg1");
    bufPush("msg2");
    bufPush("msg3");
    TEST_ASSERT_EQUAL_INT(3, g_count);
    TEST_ASSERT_EQUAL_STRING("msg1", bufPeek()); bufPop();
    TEST_ASSERT_EQUAL_STRING("msg2", bufPeek()); bufPop();
    TEST_ASSERT_EQUAL_STRING("msg3", bufPeek()); bufPop();
    TEST_ASSERT_EQUAL_INT(0, g_count);
}

void test_overflow_drops_oldest() {
    bufReset();
    for (int i = 0; i < RETRY_BUF_SIZE + 1; i++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "msg%d", i);
        bufPush(msg);
    }
    // Count must be capped at RETRY_BUF_SIZE
    TEST_ASSERT_EQUAL_INT(RETRY_BUF_SIZE, g_count);
    TEST_ASSERT_TRUE(g_overflowed);
    // Oldest (msg0) was overwritten; first readable should be msg1
    TEST_ASSERT_EQUAL_STRING("msg1", bufPeek());
}

void test_full_flush_empties_buffer() {
    bufReset();
    bufPush("a"); bufPush("b"); bufPush("c");
    while (g_count > 0) bufPop();
    TEST_ASSERT_EQUAL_INT(0, g_count);
}

void test_partial_flush_preserves_remaining() {
    bufReset();
    bufPush("x1"); bufPush("x2"); bufPush("x3");
    bufPop(); // flush one
    TEST_ASSERT_EQUAL_INT(2, g_count);
    TEST_ASSERT_EQUAL_STRING("x2", bufPeek());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_push_and_fifo_order);
    RUN_TEST(test_overflow_drops_oldest);
    RUN_TEST(test_full_flush_empties_buffer);
    RUN_TEST(test_partial_flush_preserves_remaining);
    return UNITY_END();
}
