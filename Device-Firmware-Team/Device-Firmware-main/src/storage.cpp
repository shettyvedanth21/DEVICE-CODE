#include "storage.h"
#include "config.h"
#include <Preferences.h>

static Preferences prefs;

DeviceConfig Storage::load() {
    DeviceConfig cfg;
    prefs.begin("wifi", true);
    cfg.wifiSSID   = prefs.getString("ssid", "");
    cfg.wifiPass   = prefs.getString("pass", "");
    prefs.end();

    prefs.begin("mqtt", true);
    cfg.mqttServer = prefs.getString("server", DEFAULT_MQTT_SERVER);
    cfg.mqttPort   = prefs.getInt   ("port",   DEFAULT_MQTT_PORT);
    cfg.mqttUser   = prefs.getString("user",   "");
    cfg.mqttPass   = prefs.getString("mpass",  "");
    cfg.deviceID   = prefs.getString("device", DEFAULT_DEVICE_ID);
    cfg.tenantID   = prefs.getString("tenant", "");
    if (cfg.tenantID.isEmpty()) {
        cfg.tenantID = prefs.getString("orgid", DEFAULT_TENANT_ID);
    }
    prefs.end();

    // ── Legacy config migration ─────────────────────────────────────────────
    // Devices upgraded from pre-TLS firmware have LEGACY values in NVS.
    // Port 1883 is incompatible with WiFiClientSecure (TLS).
    // Server "192.168.0.4" was the old hardcoded default — never a real broker.
    // Custom server + custom non-1883 port is NEVER touched here.
    bool migrated = false;
    if (cfg.mqttPort == LEGACY_MQTT_PORT) {
        Serial.printf("[Storage] Migrating MQTT port %d -> %d (TLS required)\n",
                      LEGACY_MQTT_PORT, DEFAULT_MQTT_PORT);
        cfg.mqttPort = DEFAULT_MQTT_PORT;
        migrated = true;
    }
    if (cfg.mqttServer == LEGACY_MQTT_SERVER) {
        Serial.printf("[Storage] Migrating MQTT server %s -> %s\n",
                      LEGACY_MQTT_SERVER, DEFAULT_MQTT_SERVER);
        cfg.mqttServer = DEFAULT_MQTT_SERVER;
        migrated = true;
    }
    if (migrated) {
        Storage::save(cfg);
        Serial.println("[Storage] Legacy config migrated and saved to NVS");
    }

    return cfg;
}

void Storage::save(const DeviceConfig& cfg) {
    prefs.begin("wifi", false);
    prefs.putString("ssid", cfg.wifiSSID);
    prefs.putString("pass", cfg.wifiPass);
    prefs.end();

    prefs.begin("mqtt", false);
    prefs.putString("server", cfg.mqttServer);
    prefs.putInt   ("port",   cfg.mqttPort);
    prefs.putString("user",   cfg.mqttUser);
    prefs.putString("mpass",  cfg.mqttPass);
    prefs.putString("device", cfg.deviceID);
    prefs.putString("tenant", cfg.tenantID);
    prefs.end();
}

void Storage::clearAll() {
    prefs.begin("wifi", false); prefs.clear(); prefs.end();
    prefs.begin("mqtt", false); prefs.clear(); prefs.end();
}
