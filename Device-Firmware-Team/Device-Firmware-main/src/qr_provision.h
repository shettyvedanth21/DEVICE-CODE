/**
 * qr_provision.h — QR provisioning payload parser.
 * Validates the JSON contract from scanned QR codes and applies
 * the parsed fields to DeviceConfig.  Wi-Fi fields are only
 * applied when explicitly present in the payload.
 *
 * Payload contract (v1):
 *   Required: v, tenant_id, device_id, mqtt_host, mqtt_port,
 *             mqtt_username, mqtt_password
 *   Optional: wifi_ssid, wifi_password
 *
 * mqtt_password is required because QR provisioning is a "scan and go"
 * flow for first-time device setup — without it the device cannot
 * authenticate to the MQTT broker.  Partial updates should use the
 * manual form (/saveall), not this endpoint.
 */
#pragma once
#include <Arduino.h>
#include "storage.h"

#define PROVISION_PAYLOAD_MAX 1024

struct QrPayload {
    int    version;
    String tenantId;
    String deviceId;
    String mqttHost;
    int    mqttPort;
    String mqttUsername;
    String mqttPassword;
    String wifiSsid;
    String wifiPassword;

    QrPayload() : version(0), mqttPort(0) {}
};

namespace QrProvision {
    bool parsePayload(const String& json, QrPayload& out, String& error);
    void applyToConfig(const QrPayload& payload, DeviceConfig& cfg);
}
