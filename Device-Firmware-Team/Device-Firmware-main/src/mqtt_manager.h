/**
 * mqtt_manager.h — MQTT connect, publish, exponential backoff, retry buffer.
 * Retry buffer uses fixed char arrays (no String heap fragmentation).
 * Uses TLS (WiFiClientSecure) and per-device MQTT auth.
 */
#pragma once
#include <Arduino.h>

namespace MqttManager {
    void init(const String& server, int port,
              const String& deviceID, const String& tenantID,
              const String& mqttUser, const String& mqttPass);
    void tick();
    bool publish(const char* payload, size_t len);
    bool isConnected();
}
