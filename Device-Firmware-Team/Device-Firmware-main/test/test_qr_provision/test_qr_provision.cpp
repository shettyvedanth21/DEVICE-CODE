/**
 * test_qr_provision.cpp — Tests for QR provisioning payload parser.
 * Validates the JSON contract, required/optional fields, format
 * checks, and the applyToConfig merge logic.
 *
 * Mirrors QrProvision::parsePayload and QrProvision::applyToConfig
 * with std::string for native test compatibility (no Arduino String).
 *
 * Run with: pio test -e native -f test_qr_provision
 */
#include <unity.h>
#include <ArduinoJson.h>
#include <string>
#include <cstdint>
#include <cctype>

#define PROVISION_PAYLOAD_MAX 1024

struct DeviceConfig {
    std::string wifiSSID;
    std::string wifiPass;
    std::string mqttServer;
    int         mqttPort;
    std::string mqttUser;
    std::string mqttPass;
    std::string deviceID;
    std::string tenantID;
    DeviceConfig() : mqttPort(0) {}
};

struct QrPayload {
    int         version;
    std::string tenantId;
    std::string deviceId;
    std::string mqttHost;
    int         mqttPort;
    std::string mqttUsername;
    std::string mqttPassword;
    std::string wifiSsid;
    std::string wifiPassword;
    QrPayload() : version(0), mqttPort(0) {}
};

static bool parsePayload(const std::string& json, QrPayload& out, std::string& error) {
    if ((int)json.length() > PROVISION_PAYLOAD_MAX) {
        error = "Payload too large"; return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError deserErr = deserializeJson(doc, json);
    if (deserErr) { error = "Invalid JSON"; return false; }

    out = QrPayload();
    out.version = doc["v"] | 0;
    if (out.version != 1) { error = "Unsupported payload version"; return false; }

    if (!doc.containsKey("tenant_id")) { error = "Missing tenant_id"; return false; }
    out.tenantId = doc["tenant_id"].as<const char*>();

    if (!doc.containsKey("device_id")) { error = "Missing device_id"; return false; }
    out.deviceId = doc["device_id"].as<const char*>();
    if (out.deviceId.length() > 32) { error = "Device ID too long"; return false; }
    for (size_t i = 0; i < out.deviceId.length(); i++) {
        char c = out.deviceId[i];
        if (!isalnum((unsigned char)c) && c != '-' && c != '_') {
            error = "Invalid character in device ID"; return false;
        }
    }

    if (!doc.containsKey("mqtt_host")) { error = "Missing mqtt_host"; return false; }
    out.mqttHost = doc["mqtt_host"].as<const char*>();

    if (!doc.containsKey("mqtt_port")) { error = "Missing mqtt_port"; return false; }
    out.mqttPort = doc["mqtt_port"].as<int>();
    if (out.mqttPort < 1 || out.mqttPort > 65535) {
        error = "MQTT port out of range (1-65535)"; return false;
    }

    if (!doc.containsKey("mqtt_username")) { error = "Missing mqtt_username"; return false; }
    out.mqttUsername = doc["mqtt_username"].as<const char*>();

    if (!doc.containsKey("mqtt_password") ||
        doc["mqtt_password"].as<const char*>()[0] == '\0') {
        error = "Missing mqtt_password"; return false;
    }
    out.mqttPassword = doc["mqtt_password"].as<const char*>();

    if (doc.containsKey("wifi_ssid"))      out.wifiSsid     = doc["wifi_ssid"].as<const char*>();
    if (doc.containsKey("wifi_password"))  out.wifiPassword = doc["wifi_password"].as<const char*>();

    return true;
}

static void applyToConfig(const QrPayload& payload, DeviceConfig& cfg) {
    cfg.tenantID   = payload.tenantId;
    cfg.deviceID   = payload.deviceId;
    cfg.mqttServer = payload.mqttHost;
    cfg.mqttPort   = payload.mqttPort;
    cfg.mqttUser   = payload.mqttUsername;
    if (!payload.mqttPassword.empty()) cfg.mqttPass = payload.mqttPassword;
    if (!payload.wifiSsid.empty())     cfg.wifiSSID = payload.wifiSsid;
    if (!payload.wifiPassword.empty()) cfg.wifiPass = payload.wifiPassword;
}

static std::string makeValidPayload() {
    return R"({"v":1,"tenant_id":"SH00000001","device_id":"AD00000001",)"
           R"("mqtt_host":"shivex.ai","mqtt_port":8883,)"
           R"("mqtt_username":"device:SH00000001:AD00000001",)"
           R"("mqtt_password":"one-time-secret"})";
}

static std::string makeValidPayloadNoWifi() {
    return R"({"v":1,"tenant_id":"SH00000002","device_id":"AD00000002",)"
           R"("mqtt_host":"broker.example.com","mqtt_port":8884,)"
           R"("mqtt_username":"user2","mqtt_password":"pw2"})";
}

// ════════════════════════════════════════════════════════════════════════
//  parsePayload tests
// ════════════════════════════════════════════════════════════════════════

void test_parse_valid_full_payload() {
    QrPayload out; std::string err;
    TEST_ASSERT_TRUE(parsePayload(makeValidPayload(), out, err));
    TEST_ASSERT_EQUAL_INT(1, out.version);
    TEST_ASSERT_EQUAL_STRING("SH00000001", out.tenantId.c_str());
    TEST_ASSERT_EQUAL_STRING("AD00000001", out.deviceId.c_str());
    TEST_ASSERT_EQUAL_STRING("shivex.ai", out.mqttHost.c_str());
    TEST_ASSERT_EQUAL_INT(8883, out.mqttPort);
    TEST_ASSERT_EQUAL_STRING("device:SH00000001:AD00000001", out.mqttUsername.c_str());
    TEST_ASSERT_EQUAL_STRING("one-time-secret", out.mqttPassword.c_str());
    TEST_ASSERT_TRUE(out.wifiSsid.empty());
    TEST_ASSERT_TRUE(out.wifiPassword.empty());
}

void test_parse_valid_mqtt_only_no_wifi() {
    QrPayload out; std::string err;
    TEST_ASSERT_TRUE(parsePayload(makeValidPayloadNoWifi(), out, err));
    TEST_ASSERT_EQUAL_STRING("pw2", out.mqttPassword.c_str());
    TEST_ASSERT_TRUE(out.wifiSsid.empty());
    TEST_ASSERT_TRUE(out.wifiPassword.empty());
}

void test_parse_with_wifi_fields() {
    std::string json = R"({"v":1,"tenant_id":"SH00000003","device_id":"AD00000003",)"
                       R"("mqtt_host":"shivex.ai","mqtt_port":8883,)"
                       R"("mqtt_username":"u3","mqtt_password":"pw3",)"
                       R"("wifi_ssid":"OfficeNet","wifi_password":"offic3pass"})";
    QrPayload out; std::string err;
    TEST_ASSERT_TRUE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("OfficeNet", out.wifiSsid.c_str());
    TEST_ASSERT_EQUAL_STRING("offic3pass", out.wifiPassword.c_str());
}

void test_parse_invalid_json() {
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload("not json", out, err));
    TEST_ASSERT_EQUAL_STRING("Invalid JSON", err.c_str());
}

void test_parse_payload_too_large() {
    std::string big(PROVISION_PAYLOAD_MAX + 100, 'X');
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(big, out, err));
    TEST_ASSERT_EQUAL_STRING("Payload too large", err.c_str());
}

void test_parse_missing_version() {
    std::string json = R"({"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Unsupported payload version", err.c_str());
}

void test_parse_wrong_version() {
    std::string json = R"({"v":2,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Unsupported payload version", err.c_str());
}

void test_parse_missing_tenant_id() {
    std::string json = R"({"v":1,"device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Missing tenant_id", err.c_str());
}

void test_parse_missing_device_id() {
    std::string json = R"({"v":1,"tenant_id":"SH01",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Missing device_id", err.c_str());
}

void test_parse_missing_mqtt_host() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Missing mqtt_host", err.c_str());
}

void test_parse_missing_mqtt_port() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Missing mqtt_port", err.c_str());
}

void test_parse_missing_mqtt_username() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Missing mqtt_username", err.c_str());
}

void test_parse_missing_mqtt_password() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Missing mqtt_password", err.c_str());
}

void test_parse_empty_mqtt_password() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":""})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Missing mqtt_password", err.c_str());
}

void test_parse_port_out_of_range_low() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":0,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("MQTT port out of range (1-65535)", err.c_str());
}

void test_parse_port_out_of_range_high() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":70000,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("MQTT port out of range (1-65535)", err.c_str());
}

void test_parse_port_boundary_low() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":1,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_TRUE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_INT(1, out.mqttPort);
}

void test_parse_port_boundary_high() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":65535,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_TRUE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_INT(65535, out.mqttPort);
}

void test_parse_device_id_too_long() {
    std::string longId(33, 'A');
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":")" + longId + R"(",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Device ID too long", err.c_str());
}

void test_parse_device_id_invalid_char() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD#001",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_FALSE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("Invalid character in device ID", err.c_str());
}

void test_parse_device_id_with_hyphen() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD-00001",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_TRUE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("AD-00001", out.deviceId.c_str());
}

void test_parse_device_id_with_underscore() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD_00001",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p"})";
    QrPayload out; std::string err;
    TEST_ASSERT_TRUE(parsePayload(json, out, err));
    TEST_ASSERT_EQUAL_STRING("AD_00001", out.deviceId.c_str());
}

void test_parse_all_six_required_fields_enforced() {
    std::string base = R"({"v":1,"tenant_id":"T","device_id":"D","mqtt_host":"H","mqtt_port":8883,)"
                       R"("mqtt_username":"U","mqtt_password":"P"})";
    QrPayload out; std::string err;
    TEST_ASSERT_TRUE(parsePayload(base, out, err));

    const char* fields[] = {"tenant_id","device_id","mqtt_host","mqtt_port","mqtt_username","mqtt_password"};
    for (int i = 0; i < 6; i++) {
        std::string reduced = base;
        std::string needle = std::string("\"") + fields[i] + "\"";
        size_t pos = reduced.find(needle);
        if (pos != std::string::npos) reduced.erase(pos, needle.length() + 4);
        TEST_ASSERT_FALSE(parsePayload(reduced, out, err));
    }
}

// ════════════════════════════════════════════════════════════════════════
//  applyToConfig tests
// ════════════════════════════════════════════════════════════════════════

void test_apply_mqtt_fields() {
    QrPayload p; std::string err;
    parsePayload(makeValidPayload(), p, err);
    DeviceConfig cfg;
    applyToConfig(p, cfg);
    TEST_ASSERT_EQUAL_STRING("SH00000001", cfg.tenantID.c_str());
    TEST_ASSERT_EQUAL_STRING("AD00000001", cfg.deviceID.c_str());
    TEST_ASSERT_EQUAL_STRING("shivex.ai", cfg.mqttServer.c_str());
    TEST_ASSERT_EQUAL_INT(8883, cfg.mqttPort);
    TEST_ASSERT_EQUAL_STRING("device:SH00000001:AD00000001", cfg.mqttUser.c_str());
    TEST_ASSERT_EQUAL_STRING("one-time-secret", cfg.mqttPass.c_str());
}

void test_apply_preserves_wifi_when_absent() {
    QrPayload p; std::string err;
    parsePayload(makeValidPayload(), p, err);
    DeviceConfig cfg;
    cfg.wifiSSID = "ExistingSSID";
    cfg.wifiPass = "ExistingPass";
    applyToConfig(p, cfg);
    TEST_ASSERT_EQUAL_STRING("ExistingSSID", cfg.wifiSSID.c_str());
    TEST_ASSERT_EQUAL_STRING("ExistingPass", cfg.wifiPass.c_str());
}

void test_apply_overwrites_wifi_when_present() {
    std::string json = R"({"v":1,"tenant_id":"SH01","device_id":"AD01",)"
                       R"("mqtt_host":"h","mqtt_port":8883,"mqtt_username":"u","mqtt_password":"p",)"
                       R"("wifi_ssid":"NewSSID","wifi_password":"NewPass"})";
    QrPayload p; std::string err;
    parsePayload(json, p, err);
    DeviceConfig cfg;
    cfg.wifiSSID = "OldSSID";
    cfg.wifiPass = "OldPass";
    applyToConfig(p, cfg);
    TEST_ASSERT_EQUAL_STRING("NewSSID", cfg.wifiSSID.c_str());
    TEST_ASSERT_EQUAL_STRING("NewPass", cfg.wifiPass.c_str());
}

void test_apply_always_writes_mqtt_password() {
    QrPayload p; std::string err;
    parsePayload(makeValidPayload(), p, err);
    DeviceConfig cfg;
    cfg.mqttPass = "old-secret";
    applyToConfig(p, cfg);
    TEST_ASSERT_EQUAL_STRING("one-time-secret", cfg.mqttPass.c_str());
}

void test_apply_port_8883_maintains_tls() {
    QrPayload p; std::string err;
    parsePayload(makeValidPayload(), p, err);
    DeviceConfig cfg;
    applyToConfig(p, cfg);
    TEST_ASSERT_EQUAL_INT(8883, cfg.mqttPort);
    TEST_ASSERT_TRUE(cfg.mqttPort != 1883);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_parse_valid_full_payload);
    RUN_TEST(test_parse_valid_mqtt_only_no_wifi);
    RUN_TEST(test_parse_with_wifi_fields);
    RUN_TEST(test_parse_invalid_json);
    RUN_TEST(test_parse_payload_too_large);
    RUN_TEST(test_parse_missing_version);
    RUN_TEST(test_parse_wrong_version);
    RUN_TEST(test_parse_missing_tenant_id);
    RUN_TEST(test_parse_missing_device_id);
    RUN_TEST(test_parse_missing_mqtt_host);
    RUN_TEST(test_parse_missing_mqtt_port);
    RUN_TEST(test_parse_missing_mqtt_username);
    RUN_TEST(test_parse_missing_mqtt_password);
    RUN_TEST(test_parse_empty_mqtt_password);
    RUN_TEST(test_parse_port_out_of_range_low);
    RUN_TEST(test_parse_port_out_of_range_high);
    RUN_TEST(test_parse_port_boundary_low);
    RUN_TEST(test_parse_port_boundary_high);
    RUN_TEST(test_parse_device_id_too_long);
    RUN_TEST(test_parse_device_id_invalid_char);
    RUN_TEST(test_parse_device_id_with_hyphen);
    RUN_TEST(test_parse_device_id_with_underscore);
    RUN_TEST(test_parse_all_six_required_fields_enforced);

    RUN_TEST(test_apply_mqtt_fields);
    RUN_TEST(test_apply_preserves_wifi_when_absent);
    RUN_TEST(test_apply_overwrites_wifi_when_present);
    RUN_TEST(test_apply_always_writes_mqtt_password);
    RUN_TEST(test_apply_port_8883_maintains_tls);

    return UNITY_END();
}
