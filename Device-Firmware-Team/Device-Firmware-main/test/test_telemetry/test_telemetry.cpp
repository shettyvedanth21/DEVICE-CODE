/**
 * test_telemetry.cpp — Schema v2 payload structure tests.
 * Inlines buildPayload logic (matches pattern of other test files) to
 * avoid Arduino platform dependencies in the native test environment.
 *
 * Validates:
 *   - Required top-level fields present (device_id, tenant_id, timestamp, schema_version)
 *   - device_metadata sub-object contains relocated non-numeric fields
 *   - No invalid non-numeric fields at top level
 *   - All numeric telemetry fields present and match
 *   - Payload fits within bounds
 *
 * Run with: pio test -e native
 */
#include <unity.h>
#include <ArduinoJson.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// ── Constants (mirror of config.h — no Arduino.h dependency) ─────────────
#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION "2.0.0"
#endif
#define MQTT_PAYLOAD_MAX 640
#ifdef ARDUINO
  #define JSON_DOC_SIZE 768
#else
  #define JSON_DOC_SIZE 2048
#endif

// ── MeterReading struct (mirror of meter_data.h) ──────────────────────────
struct MeterReading {
    float    v1        = 0.0f;
    float    v2        = 0.0f;
    float    v3        = 0.0f;
    float    vAvg      = 0.0f;
    float    vLine     = 0.0f;
    float    i1        = 0.0f;
    float    i2        = 0.0f;
    float    i3        = 0.0f;
    float    iAvg      = 0.0f;
    float    powerW    = 0.0f;
    float    energyKwh = 0.0f;
    float    pf        = 0.0f;
    float    freqHz    = 0.0f;
    float    kvar      = 0.0f;
    float    kvah      = 0.0f;
    float    runHours  = 0.0f;
    uint16_t modbusErrors = 0;
    uint32_t meterSerial  = 0;
};

// ── Inline mirror of Telemetry::buildPayload (production-aligned) ──────────
static size_t buildPayload(
    const MeterReading& m,
    const char*  deviceID,
    const char*  tenantID,
    const char*  timestamp,
    const char*  bootReason,
    int          wifiRssi,
    uint32_t     uptimeS,
    char*        buf,
    size_t       bufLen)
{
    DynamicJsonDocument doc(JSON_DOC_SIZE);

    doc["schema_version"]   = "v2";
    doc["device_id"]        = deviceID;
    doc["tenant_id"]        = tenantID;
    doc["timestamp"]        = timestamp;

    JsonObject meta = doc.createNestedObject("device_metadata");
    meta["firmware_version"] = FIRMWARE_VERSION;
    meta["boot_reason"]      = bootReason;
    meta["meter_serial"]     = m.meterSerial;

    doc["uptime_s"]         = uptimeS;
    doc["wifi_rssi"]        = wifiRssi;
    doc["modbus_errors"]    = m.modbusErrors;

    doc["voltage_l1"]       = m.v1;
    doc["voltage_l2"]       = m.v2;
    doc["voltage_l3"]       = m.v3;
    doc["voltage_avg"]      = m.vAvg;
    doc["voltage_line"]     = m.vLine;

    doc["current_l1"]       = m.i1;
    doc["current_l2"]       = m.i2;
    doc["current_l3"]       = m.i3;
    doc["current_avg"]      = m.iAvg;

    doc["power_w"]          = m.powerW;
    doc["energy_kwh"]       = m.energyKwh;
    doc["power_factor"]     = m.pf;
    doc["frequency_hz"]     = m.freqHz;
    doc["kvar"]             = m.kvar;
    doc["kvah"]             = m.kvah;
    doc["run_hours"]        = m.runHours;

    if (doc.overflowed()) return 0;
    size_t len = serializeJson(doc, buf, bufLen);
    if (len == 0 || len >= bufLen) return 0;
    return len;
}

// ── Shared test payload ───────────────────────────────────────────────────
static MeterReading makeSentinel() {
    MeterReading m;
    m.v1 = 231.4f; m.v2 = 230.8f; m.v3 = 231.1f;
    m.vAvg = 231.1f; m.vLine = 400.2f;
    m.i1 = 4.2f; m.i2 = 4.1f; m.i3 = 4.3f; m.iAvg = 4.2f;
    m.powerW = 2916.0f; m.energyKwh = 148.3f;
    m.pf = 0.94f; m.freqHz = 49.98f;
    m.kvar = 0.38f; m.kvah = 3.1f; m.runHours = 423.5f;
    m.modbusErrors = 0; m.meterSerial = 12345678;
    return m;
}

// ── Helper: parse payload into doc, with assertion ─────────────────────────
static bool parsePayload(const char* buf, size_t len, DynamicJsonDocument& doc) {
    DeserializationError err = deserializeJson(doc, buf, len);
    return err == DeserializationError::Ok;
}

// ══════════════════════════════════════════════════════════════════════════
//  Required top-level fields
// ══════════════════════════════════════════════════════════════════════════

void test_required_fields_present() {
    MeterReading m = makeSentinel();
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "TD00000007", "tenant-abc",
                              "2026-04-20T07:30:00Z", "POWER_ON",
                              -68, 3600, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    TEST_ASSERT_TRUE(parsePayload(buf, len, doc));

    TEST_ASSERT_TRUE(doc.containsKey("device_id"));
    TEST_ASSERT_TRUE(doc.containsKey("tenant_id"));
    TEST_ASSERT_TRUE(doc.containsKey("timestamp"));
    TEST_ASSERT_TRUE(doc.containsKey("schema_version"));

    TEST_ASSERT_EQUAL_STRING("TD00000007", doc["device_id"]);
    TEST_ASSERT_EQUAL_STRING("tenant-abc", doc["tenant_id"]);
    TEST_ASSERT_EQUAL_STRING("2026-04-20T07:30:00Z", doc["timestamp"]);
    TEST_ASSERT_EQUAL_STRING("v2", doc["schema_version"]);
}

// ══════════════════════════════════════════════════════════════════════════
//  device_metadata sub-object
// ══════════════════════════════════════════════════════════════════════════

void test_device_metadata_present() {
    MeterReading m = makeSentinel();
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "TD00000007", "tenant-abc",
                              "2026-04-20T07:30:00Z", "POWER_ON",
                              -68, 3600, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    TEST_ASSERT_TRUE(parsePayload(buf, len, doc));

    TEST_ASSERT_TRUE(doc.containsKey("device_metadata"));
    TEST_ASSERT_TRUE(doc["device_metadata"].is<JsonObject>());

    JsonObject meta = doc["device_metadata"];
    TEST_ASSERT_TRUE(meta.containsKey("firmware_version"));
    TEST_ASSERT_TRUE(meta.containsKey("boot_reason"));
    TEST_ASSERT_TRUE(meta.containsKey("meter_serial"));

    TEST_ASSERT_EQUAL_STRING("POWER_ON", meta["boot_reason"]);
    TEST_ASSERT_EQUAL_UINT32(12345678u, meta["meter_serial"].as<uint32_t>());
}

// ══════════════════════════════════════════════════════════════════════════
//  No invalid non-numeric fields at top level
// ══════════════════════════════════════════════════════════════════════════

void test_no_org_id_at_top_level() {
    MeterReading m = makeSentinel();
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "TD00000007", "tenant-abc",
                              "2026-04-20T07:30:00Z", "POWER_ON",
                              -68, 3600, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    TEST_ASSERT_TRUE(parsePayload(buf, len, doc));
    TEST_ASSERT_FALSE(doc.containsKey("org_id"));
}

void test_no_firmware_version_at_top_level() {
    MeterReading m = makeSentinel();
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "TD00000007", "tenant-abc",
                              "2026-04-20T07:30:00Z", "POWER_ON",
                              -68, 3600, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    TEST_ASSERT_TRUE(parsePayload(buf, len, doc));
    TEST_ASSERT_FALSE(doc.containsKey("firmware_version"));
}

void test_no_boot_reason_at_top_level() {
    MeterReading m = makeSentinel();
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "TD00000007", "tenant-abc",
                              "2026-04-20T07:30:00Z", "POWER_ON",
                              -68, 3600, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    TEST_ASSERT_TRUE(parsePayload(buf, len, doc));
    TEST_ASSERT_FALSE(doc.containsKey("boot_reason"));
}

void test_no_meter_serial_at_top_level() {
    MeterReading m = makeSentinel();
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "TD00000007", "tenant-abc",
                              "2026-04-20T07:30:00Z", "POWER_ON",
                              -68, 3600, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    TEST_ASSERT_TRUE(parsePayload(buf, len, doc));
    TEST_ASSERT_FALSE(doc.containsKey("meter_serial"));
}

// ══════════════════════════════════════════════════════════════════════════
//  Numeric telemetry fields
// ══════════════════════════════════════════════════════════════════════════

void test_numeric_telemetry_fields_match() {
    MeterReading m = makeSentinel();
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "TD00000007", "tenant-abc",
                              "2026-04-20T07:30:00Z", "POWER_ON",
                              -68, 3600, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    TEST_ASSERT_TRUE(parsePayload(buf, len, doc));

    // Diagnostic numeric
    TEST_ASSERT_EQUAL_UINT32(3600u,  doc["uptime_s"].as<uint32_t>());
    TEST_ASSERT_EQUAL_INT(-68,       doc["wifi_rssi"].as<int>());
    TEST_ASSERT_EQUAL_UINT16(0,      doc["modbus_errors"].as<uint16_t>());

    // Voltage (5)
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 231.4f, doc["voltage_l1"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 230.8f, doc["voltage_l2"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 231.1f, doc["voltage_l3"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 231.1f, doc["voltage_avg"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 400.2f, doc["voltage_line"].as<float>());

    // Current (4)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.2f, doc["current_l1"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.1f, doc["current_l2"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.3f, doc["current_l3"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.2f, doc["current_avg"].as<float>());

    // Power & energy (7)
    TEST_ASSERT_FLOAT_WITHIN(0.1f,  2916.0f, doc["power_w"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.1f,  148.3f,  doc["energy_kwh"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.94f,  doc["power_factor"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f,  49.98f, doc["frequency_hz"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f,  0.38f,  doc["kvar"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f,  3.1f,   doc["kvah"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.1f,  423.5f,  doc["run_hours"].as<float>());
}

// ══════════════════════════════════════════════════════════════════════════
//  Bounds and overflow
// ══════════════════════════════════════════════════════════════════════════

void test_payload_length_within_bounds() {
    MeterReading m = makeSentinel();
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "TD00000007", "186aa55-5096-44f3-8de4-aa6549173836",
                              "2026-04-20T07:30:00Z", "POWER_ON",
                              -68, 9999999, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);
    TEST_ASSERT_LESS_THAN((size_t)MQTT_PAYLOAD_MAX, len);
}

void test_zero_meter_reading_produces_valid_json() {
    MeterReading m;
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "D1", "O1", "2026-01-01T00:00:00Z",
                              "POWER_ON", -90, 0, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    TEST_ASSERT_TRUE(parsePayload(buf, len, doc));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, doc["voltage_l1"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, doc["power_w"].as<float>());
}

void test_schema_version_is_v2() {
    MeterReading m = makeSentinel();
    char buf[MQTT_PAYLOAD_MAX];
    size_t len = buildPayload(m, "D1", "O1", "2026-01-01T00:00:00Z",
                              "POWER_ON", -70, 0, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0u, len);

    DynamicJsonDocument doc(JSON_DOC_SIZE);
    TEST_ASSERT_TRUE(parsePayload(buf, len, doc));
    TEST_ASSERT_EQUAL_STRING("v2", doc["schema_version"]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_required_fields_present);
    RUN_TEST(test_device_metadata_present);
    RUN_TEST(test_no_org_id_at_top_level);
    RUN_TEST(test_no_firmware_version_at_top_level);
    RUN_TEST(test_no_boot_reason_at_top_level);
    RUN_TEST(test_no_meter_serial_at_top_level);
    RUN_TEST(test_numeric_telemetry_fields_match);
    RUN_TEST(test_payload_length_within_bounds);
    RUN_TEST(test_zero_meter_reading_produces_valid_json);
    RUN_TEST(test_schema_version_is_v2);
    return UNITY_END();
}
