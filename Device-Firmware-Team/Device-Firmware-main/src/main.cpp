/**
 * main.cpp — Shivex.ai Device Firmware v2.0
 *
 * setup() and loop() only. Zero business logic here.
 * All logic lives in the 11 modules under src/.
 *
 * Loop order (each step must return in <50ms total per iteration):
 *   1. Watchdog feed
 *   2. Deferred restart check
 *   3. Factory reset button (millis()-gated)
 *   4. Web portal (server.handleClient)
 *   5. WiFi FSM
 *   6. LED updates
 *   [guard: return if WiFi not connected]
 *   7. NTP tick
 *   8. Modbus (one register pair)
 *   9. MQTT tick
 *  10. OTA (ArduinoOTA.handle)
 *  11. Publish on interval (millis()-gated)
 */
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "meter_data.h"
#include "storage.h"
#include "watchdog.h"
#include "led_manager.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "modbus_reader.h"
#include "ntp_client.h"
#include "web_portal.h"
#include "telemetry.h"
#include "ota_manager.h"

volatile MeterReading g_meter;

static DeviceConfig  g_cfg;
static const char*   g_bootReason   = "UNKNOWN";
static bool          g_btnHeld      = false;
static unsigned long g_btnPressStart = 0;
static unsigned long g_lastPublishMs = 0;
static char          g_payload[MQTT_PAYLOAD_MAX];

void setup() {
    Serial.begin(115200);

    // 1. Watchdog — first thing, before anything else can hang
    Watchdog::init();
    g_bootReason = Watchdog::bootReasonStr();

    // 2. LEDs
    LedManager::init();

    // 3. Load config from NVS
    g_cfg = Storage::load();

    // 4. Setup button
    pinMode(PIN_SETUP_BUTTON, INPUT_PULLUP);

    // 5. Modbus — init configures Serial2; meter serial read is deferred to tick()
    ModbusReader::init();

    // 6. MQTT client — topic computed once here, never reassigned
    MqttManager::init(g_cfg.mqttServer, g_cfg.mqttPort,
                      g_cfg.deviceID,   g_cfg.tenantID,
                      g_cfg.mqttUser,   g_cfg.mqttPass);

    // 7. WiFi FSM — starts connecting or enters AP mode
    WifiManager::init(g_cfg.wifiSSID, g_cfg.wifiPass);

    // 8. Web portal — always available (AP or STA mode)
    WebPortal::init(g_cfg);

    // 9. OTA — ArduinoOTA.begin() called here; handle() called in loop
    OtaManager::init(g_cfg.deviceID);

    // ── Structured boot log ───────────────────────────────────────────────
    String topic = g_cfg.tenantID + "/devices/" + g_cfg.deviceID + "/telemetry";
    Serial.println("─────────── Shivex Device Boot ───────────");
    Serial.printf("Firmware   : %s\n",    FIRMWARE_VERSION);
    Serial.printf("Device ID  : %s\n",    g_cfg.deviceID.c_str());
    Serial.printf("Tenant ID  : %s\n",    g_cfg.tenantID.c_str());
    Serial.printf("Chip ID    : %016llX\n", ESP.getEfuseMac());
    Serial.printf("MQTT       : %s:%d\n", g_cfg.mqttServer.c_str(), g_cfg.mqttPort);
    Serial.printf("Topic      : %s\n",    topic.c_str());
    Serial.printf("Boot reason: %s\n",    g_bootReason);
#if MQTT_SKIP_CERT_VERIFY
    Serial.println("!!! TLS VERIFY: DISABLED (localdev build — not for production) !!!");
#endif
    Serial.println("───────────────────────────────────────────");
}

void loop() {
    // ── 1. Watchdog — always first ────────────────────────────────────────
    Watchdog::feed();

    // ── 2. Deferred restart after web config save ─────────────────────────
    if (WebPortal::pendingRestart &&
        millis() - WebPortal::restartFlagTime > 200UL) {
        Serial.println("[Main] Restarting after config save...");
        ESP.restart();
    }

    // ── 3. Factory reset — hold SETUP_BUTTON for FACTORY_RESET_HOLD_MS ───
    if (digitalRead(PIN_SETUP_BUTTON) == LOW) {
        if (!g_btnHeld) {
            g_btnHeld        = true;
            g_btnPressStart  = millis();
        } else if (millis() - g_btnPressStart > FACTORY_RESET_HOLD_MS) {
            Serial.println("[Main] Factory reset triggered — clearing NVS");
            Storage::clearAll();
            ESP.restart();
        }
    } else {
        g_btnHeld = false;
    }

    // ── 4. Web portal ─────────────────────────────────────────────────────
    WebPortal::tick();

    // ── 5. WiFi FSM ───────────────────────────────────────────────────────
    WifiManager::tick();

    // ── 6. LED updates ────────────────────────────────────────────────────
    LedManager::tick();

    // Guard: cloud services require WiFi
    if (!WifiManager::isConnected()) return;

    // ── 7. NTP sync check ─────────────────────────────────────────────────
    NtpClient::tick();

    // ── 8. Modbus — one register pair per loop ────────────────────────────
    ModbusReader::tick();

    // ── 9. MQTT connect/reconnect with exponential backoff ────────────────
    MqttManager::tick();

    // ── 10. OTA ───────────────────────────────────────────────────────────
    OtaManager::tick();

    // ── 11. Publish on interval ───────────────────────────────────────────
    if (millis() - g_lastPublishMs < PUBLISH_INTERVAL_MS) return;
    if (!NtpClient::isReady()) return;   // wait for NTP without blocking

    g_lastPublishMs = millis();

    String ts = NtpClient::getISOTimeUTC();
    MeterReading snapshot = const_cast<MeterReading&>(g_meter);

    size_t len = Telemetry::buildPayload(
        snapshot,
        g_cfg.deviceID.c_str(),
        g_cfg.tenantID.c_str(),
        ts.c_str(),
        g_bootReason,
        WiFi.RSSI(),
        (uint32_t)(millis() / 1000UL),
        g_payload,
        sizeof(g_payload)
    );

    if (len == 0) {
        Serial.println("[Main] Payload build failed — skipping publish");
        return;
    }

    bool ok = MqttManager::publish(g_payload, len);
    Serial.printf("[Main] Publish %s (%u bytes) — %s\n",
                  ok ? "OK" : "queued", (unsigned)len, ts.c_str());
}
