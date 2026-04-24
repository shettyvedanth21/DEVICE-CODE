/**
 * storage.h — Single Preferences wrapper.
 * All NVS reads/writes go through this module only.
 * No other module calls preferences.begin() directly.
 */
#pragma once
#include <Arduino.h>

struct DeviceConfig {
    String wifiSSID;
    String wifiPass;
    String mqttServer;
    int    mqttPort;
    String mqttUser;
    String mqttPass;
    String deviceID;
    String tenantID;
};

namespace Storage {
    DeviceConfig load();
    void         save(const DeviceConfig& cfg);
    void         clearAll();
}
