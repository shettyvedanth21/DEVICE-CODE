#include "qr_provision.h"
#include "config.h"
#include <ArduinoJson.h>

bool QrProvision::parsePayload(const String& json, QrPayload& out, String& error) {
    if ((int)json.length() > PROVISION_PAYLOAD_MAX) {
        error = F("Payload too large");
        return false;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<512> doc;
#endif
    DeserializationError deserErr = deserializeJson(doc, json);
    if (deserErr) {
        error = F("Invalid JSON");
        return false;
    }

    out = QrPayload();

    out.version = doc["v"] | 0;
    if (out.version != 1) {
        error = F("Unsupported payload version");
        return false;
    }

    if (!doc.containsKey("tenant_id")) { error = F("Missing tenant_id"); return false; }
    out.tenantId = doc["tenant_id"].as<String>();

    if (!doc.containsKey("device_id")) { error = F("Missing device_id"); return false; }
    out.deviceId = doc["device_id"].as<String>();
    if (out.deviceId.length() > 32) { error = F("Device ID too long"); return false; }
    for (unsigned int i = 0; i < out.deviceId.length(); i++) {
        char c = out.deviceId[i];
        if (!isAlphaNumeric(c) && c != '-' && c != '_') {
            error = F("Invalid character in device ID");
            return false;
        }
    }

    if (!doc.containsKey("mqtt_host")) { error = F("Missing mqtt_host"); return false; }
    out.mqttHost = doc["mqtt_host"].as<String>();

    if (!doc.containsKey("mqtt_port")) { error = F("Missing mqtt_port"); return false; }
    out.mqttPort = doc["mqtt_port"].as<int>();
    if (out.mqttPort < 1 || out.mqttPort > 65535) {
        error = F("MQTT port out of range (1-65535)");
        return false;
    }

    if (!doc.containsKey("mqtt_username")) { error = F("Missing mqtt_username"); return false; }
    out.mqttUsername = doc["mqtt_username"].as<String>();

    if (!doc.containsKey("mqtt_password") || doc["mqtt_password"].as<String>().isEmpty()) {
        error = F("Missing mqtt_password");
        return false;
    }
    out.mqttPassword = doc["mqtt_password"].as<String>();

    if (doc.containsKey("wifi_ssid"))      out.wifiSsid     = doc["wifi_ssid"].as<String>();
    if (doc.containsKey("wifi_password"))  out.wifiPassword = doc["wifi_password"].as<String>();

    return true;
}

void QrProvision::applyToConfig(const QrPayload& payload, DeviceConfig& cfg) {
    cfg.tenantID   = payload.tenantId;
    cfg.deviceID   = payload.deviceId;
    cfg.mqttServer = payload.mqttHost;
    cfg.mqttPort   = payload.mqttPort;
    cfg.mqttUser   = payload.mqttUsername;
    if (!payload.mqttPassword.isEmpty()) cfg.mqttPass = payload.mqttPassword;
    if (!payload.wifiSsid.isEmpty())     cfg.wifiSSID = payload.wifiSsid;
    if (!payload.wifiPassword.isEmpty()) cfg.wifiPass = payload.wifiPassword;
}
