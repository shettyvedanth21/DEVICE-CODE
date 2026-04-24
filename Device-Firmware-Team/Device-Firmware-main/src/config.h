/**
 * config.h — All compile-time constants and pin definitions.
 * NO secrets. NO Arduino.h — this header must compile on native (test env).
 * Secrets live in include/secrets.h (gitignored).
 */
#pragma once
#include <stdint.h>

// ── Firmware ──────────────────────────────────────────────────────────────
#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION "2.0.0-dev"
#endif

// ── Hardware pins ─────────────────────────────────────────────────────────
#define PIN_RS485_RX      16
#define PIN_RS485_TX      17
#define PIN_RS485_DE_RE    4
#define PIN_SETUP_BUTTON  25
#define PIN_WIFI_LED       2
#define PIN_MQTT_LED       5

// ── Modbus ────────────────────────────────────────────────────────────────
#define MODBUS_SLAVE_ID        1
#define MODBUS_BAUD         9600
#define MODBUS_SERIAL_CFG   SERIAL_8N1

// MFM384 input register addresses (verified against hardware — do not reorder)
#define REG_VN1        0
#define REG_VN2        2
#define REG_VN3        4
#define REG_VN         6
#define REG_VL        14
#define REG_I1        16
#define REG_I2        18
#define REG_I3        20
#define REG_IA        22
#define REG_PA        42
#define REG_PF        54
#define REG_FREQ      56
#define REG_KWH       58
#define REG_KVAH      60
#define REG_KVAR      62
#define REG_RUNHOURS  82
#define REG_METER_SERIAL 0x02AC

// ── MQTT defaults (overridden by NVS on boot) ─────────────────────────────
#define DEFAULT_MQTT_SERVER  "shivex.ai"
#define DEFAULT_MQTT_PORT    8883
#define DEFAULT_DEVICE_ID    "TD00000007"
#define DEFAULT_TENANT_ID    ""

// ── Legacy MQTT defaults (pre-TLS firmware) ───────────────────────────────
// Used by Storage::load() to detect and auto-migrate devices that were
// running the old insecure firmware. These values must never change.
#define LEGACY_MQTT_SERVER   "192.168.0.4"
#define LEGACY_MQTT_PORT     1883

// ── MQTT TLS ──────────────────────────────────────────────────────────────
// Set to 1 only for local development against a broker without valid certs.
// Production MUST use 0 (certificate validation enforced).
#ifndef MQTT_SKIP_CERT_VERIFY
  #define MQTT_SKIP_CERT_VERIFY 0
#endif

// ── Timing ────────────────────────────────────────────────────────────────
#define PUBLISH_INTERVAL_MS     10000UL   // 10 seconds between publishes
#define MQTT_BACKOFF_MIN_MS      1000UL
#define MQTT_BACKOFF_MAX_MS     60000UL
#define WIFI_CONNECT_TIMEOUT_MS 15000UL
#define WATCHDOG_TIMEOUT_S          30
#define FACTORY_RESET_HOLD_MS    8000UL
#define AP_LED_BLINK_MS           400UL
#define MODBUS_SETTLE_MS          500UL   // RS485 transceiver settle after power-on

// ── MQTT client settings ──────────────────────────────────────────────────
#define MQTT_KEEPALIVE_S         60
#define MQTT_SOCKET_TIMEOUT_S     4
#define MQTT_PACKET_SIZE       1024
#define MQTT_PAYLOAD_MAX        640

// ── Retry buffer ──────────────────────────────────────────────────────────
#define RETRY_BUF_SIZE           5

// ── SoftAP ────────────────────────────────────────────────────────────────
#define AP_SSID   "Shivex_Setup"

// ── NTP ───────────────────────────────────────────────────────────────────
#define NTP_SERVER          "pool.ntp.org"
#define NTP_GMT_OFFSET_S    0
#define NTP_DAYLIGHT_S      0
#define NTP_VALID_EPOCH     1700000000UL
