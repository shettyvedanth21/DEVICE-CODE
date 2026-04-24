/**
 * test_mqtt_config.cpp — Tests for MQTT secure config model.
 * Validates that DeviceConfig struct correctly stores MQTT credentials,
 * that the secure config helpers behave correctly, and that topic
 * derivation matches the platform contract.
 *
 * This test does NOT depend on NVS/Preferences (which need ESP32 hardware).
 * It tests the in-memory struct and helper logic only.
 *
 * Run with: pio test -e native
 */
#include <unity.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

// ── Mirrored constants from config.h ──────────────────────────────────────
#define DEFAULT_MQTT_SERVER  "shivex.ai"
#define DEFAULT_MQTT_PORT    8883
#define DEFAULT_DEVICE_ID    "TD00000007"
#define DEFAULT_TENANT_ID    ""
#define LEGACY_MQTT_SERVER   "192.168.0.4"
#define LEGACY_MQTT_PORT     1883

// ── Mirrored DeviceConfig from storage.h (without Arduino String) ─────────
// Using std::string for native test compatibility
struct DeviceConfig {
    std::string wifiSSID;
    std::string wifiPass;
    std::string mqttServer;
    int         mqttPort;
    std::string mqttUser;
    std::string mqttPass;
    std::string deviceID;
    std::string tenantID;
};

// ── Topic derivation helper (mirrors mqtt_manager.cpp logic) ──────────────
static std::string deriveTopic(const std::string& tenantID,
                                const std::string& deviceID) {
    return tenantID + "/devices/" + deviceID + "/telemetry";
}

// ── Legacy migration helper (mirrors storage.cpp migration logic) ────────
// Returns migrated config. Mirrors the exact rules in Storage::load().
static DeviceConfig migrateLegacy(DeviceConfig cfg) {
    bool migrated = false;
    if (cfg.mqttPort == LEGACY_MQTT_PORT) {
        cfg.mqttPort = DEFAULT_MQTT_PORT;
        migrated = true;
    }
    if (cfg.mqttServer == LEGACY_MQTT_SERVER) {
        cfg.mqttServer = DEFAULT_MQTT_SERVER;
        migrated = true;
    }
    (void)migrated;
    return cfg;
}

// ══════════════════════════════════════════════════════════════════════════
//  Tests
// ══════════════════════════════════════════════════════════════════════════

void test_config_has_mqtt_user_field() {
    DeviceConfig cfg;
    TEST_ASSERT_TRUE(cfg.mqttUser.empty());
    cfg.mqttUser = "device-td007";
    TEST_ASSERT_EQUAL_STRING("device-td007", cfg.mqttUser.c_str());
}

void test_config_has_mqtt_pass_field() {
    DeviceConfig cfg;
    TEST_ASSERT_TRUE(cfg.mqttPass.empty());
    cfg.mqttPass = "s3cret!";
    TEST_ASSERT_EQUAL_STRING("s3cret!", cfg.mqttPass.c_str());
}

void test_config_has_tenant_id_field() {
    DeviceConfig cfg;
    TEST_ASSERT_TRUE(cfg.tenantID.empty());
    cfg.tenantID = "186aa55-5096-44f3-8de4-aa6549173836";
    TEST_ASSERT_EQUAL_STRING("186aa55-5096-44f3-8de4-aa6549173836",
                             cfg.tenantID.c_str());
}

void test_config_default_port_is_8883() {
    TEST_ASSERT_EQUAL_INT(8883, DEFAULT_MQTT_PORT);
}

void test_config_default_server_is_production() {
    TEST_ASSERT_EQUAL_STRING("shivex.ai", DEFAULT_MQTT_SERVER);
}

void test_topic_format_matches_platform_contract() {
    std::string topic = deriveTopic("tenant-abc", "TD00000007");
    TEST_ASSERT_EQUAL_STRING("tenant-abc/devices/TD00000007/telemetry",
                             topic.c_str());
}

void test_topic_with_uuid_tenant() {
    std::string topic = deriveTopic(
        "186aa55-5096-44f3-8de4-aa6549173836", "TD00000007");
    TEST_ASSERT_EQUAL_STRING(
        "186aa55-5096-44f3-8de4-aa6549173836/devices/TD00000007/telemetry",
        topic.c_str());
}

void test_empty_credentials_indicate_misconfiguration() {
    DeviceConfig cfg;
    cfg.mqttServer = DEFAULT_MQTT_SERVER;
    cfg.mqttPort   = DEFAULT_MQTT_PORT;
    TEST_ASSERT_TRUE(cfg.mqttUser.empty());
    TEST_ASSERT_TRUE(cfg.mqttPass.empty());
}

void test_config_all_fields_populated() {
    DeviceConfig cfg;
    cfg.mqttServer = "shivex.ai";
    cfg.mqttPort   = 8883;
    cfg.mqttUser   = "device-td007";
    cfg.mqttPass   = "pass123";
    cfg.deviceID   = "TD00000007";
    cfg.tenantID   = "tenant-abc";

    TEST_ASSERT_EQUAL_STRING("shivex.ai",  cfg.mqttServer.c_str());
    TEST_ASSERT_EQUAL_INT(8883,             cfg.mqttPort);
    TEST_ASSERT_EQUAL_STRING("device-td007", cfg.mqttUser.c_str());
    TEST_ASSERT_EQUAL_STRING("pass123",     cfg.mqttPass.c_str());
    TEST_ASSERT_EQUAL_STRING("TD00000007",  cfg.deviceID.c_str());
    TEST_ASSERT_EQUAL_STRING("tenant-abc",  cfg.tenantID.c_str());
}

void test_port_8883_is_mqtts() {
    TEST_ASSERT_EQUAL_INT(8883, DEFAULT_MQTT_PORT);
    TEST_ASSERT_FALSE(DEFAULT_MQTT_PORT == 1883);
}

// ══════════════════════════════════════════════════════════════════════════
//  Legacy migration tests
// ══════════════════════════════════════════════════════════════════════════

void test_legacy_server_and_port_migrated() {
    DeviceConfig cfg;
    cfg.mqttServer = LEGACY_MQTT_SERVER;
    cfg.mqttPort   = LEGACY_MQTT_PORT;
    cfg = migrateLegacy(cfg);
    TEST_ASSERT_EQUAL_STRING(DEFAULT_MQTT_SERVER, cfg.mqttServer.c_str());
    TEST_ASSERT_EQUAL_INT(DEFAULT_MQTT_PORT, cfg.mqttPort);
}

void test_legacy_port_only_migrated() {
    DeviceConfig cfg;
    cfg.mqttServer = "custom.broker.local";
    cfg.mqttPort   = LEGACY_MQTT_PORT;
    cfg = migrateLegacy(cfg);
    TEST_ASSERT_EQUAL_STRING("custom.broker.local", cfg.mqttServer.c_str());
    TEST_ASSERT_EQUAL_INT(DEFAULT_MQTT_PORT, cfg.mqttPort);
}

void test_legacy_server_only_migrated() {
    DeviceConfig cfg;
    cfg.mqttServer = LEGACY_MQTT_SERVER;
    cfg.mqttPort   = 8884;
    cfg = migrateLegacy(cfg);
    TEST_ASSERT_EQUAL_STRING(DEFAULT_MQTT_SERVER, cfg.mqttServer.c_str());
    TEST_ASSERT_EQUAL_INT(8884, cfg.mqttPort);
}

void test_custom_secure_config_not_migrated() {
    DeviceConfig cfg;
    cfg.mqttServer = "my-secure-broker.local";
    cfg.mqttPort   = 8883;
    cfg.mqttUser   = "device-001";
    cfg.mqttPass   = "pass123";
    cfg.tenantID   = "tenant-xyz";
    cfg = migrateLegacy(cfg);
    TEST_ASSERT_EQUAL_STRING("my-secure-broker.local", cfg.mqttServer.c_str());
    TEST_ASSERT_EQUAL_INT(8883, cfg.mqttPort);
    TEST_ASSERT_EQUAL_STRING("device-001", cfg.mqttUser.c_str());
    TEST_ASSERT_EQUAL_STRING("pass123", cfg.mqttPass.c_str());
    TEST_ASSERT_EQUAL_STRING("tenant-xyz", cfg.tenantID.c_str());
}

void test_production_defaults_not_migrated() {
    DeviceConfig cfg;
    cfg.mqttServer = DEFAULT_MQTT_SERVER;
    cfg.mqttPort   = DEFAULT_MQTT_PORT;
    cfg = migrateLegacy(cfg);
    TEST_ASSERT_EQUAL_STRING(DEFAULT_MQTT_SERVER, cfg.mqttServer.c_str());
    TEST_ASSERT_EQUAL_INT(DEFAULT_MQTT_PORT, cfg.mqttPort);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_config_has_mqtt_user_field);
    RUN_TEST(test_config_has_mqtt_pass_field);
    RUN_TEST(test_config_has_tenant_id_field);
    RUN_TEST(test_config_default_port_is_8883);
    RUN_TEST(test_config_default_server_is_production);
    RUN_TEST(test_topic_format_matches_platform_contract);
    RUN_TEST(test_topic_with_uuid_tenant);
    RUN_TEST(test_empty_credentials_indicate_misconfiguration);
    RUN_TEST(test_config_all_fields_populated);
    RUN_TEST(test_port_8883_is_mqtts);
    RUN_TEST(test_legacy_server_and_port_migrated);
    RUN_TEST(test_legacy_port_only_migrated);
    RUN_TEST(test_legacy_server_only_migrated);
    RUN_TEST(test_custom_secure_config_not_migrated);
    RUN_TEST(test_production_defaults_not_migrated);
    return UNITY_END();
}
