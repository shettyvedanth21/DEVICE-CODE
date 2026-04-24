#include "mqtt_manager.h"
#include "config.h"
#include "secrets.h"
#include "led_manager.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#if !MQTT_SKIP_CERT_VERIFY && !defined(MQTT_ROOT_CA)
#error "MQTT_ROOT_CA not defined. Copy include/secrets.h.template to include/secrets.h and fill the broker root CA certificate."
#endif

static WiFiClientSecure g_secureClient;
static PubSubClient    g_mqtt(g_secureClient);

static String g_server;
static int    g_port;
static String g_deviceID;
static String g_tenantID;
static String g_mqttUser;
static String g_mqttPass;
static String g_topic;

static char  g_retryBuf[RETRY_BUF_SIZE][MQTT_PAYLOAD_MAX];
static int   g_retryHead  = 0;
static int   g_retryCount = 0;

static unsigned long g_lastAttemptMs  = 0;
static unsigned long g_backoffDelayMs = MQTT_BACKOFF_MIN_MS;

static void retryPush(const char* payload) {
    strncpy(g_retryBuf[g_retryHead], payload, MQTT_PAYLOAD_MAX - 1);
    g_retryBuf[g_retryHead][MQTT_PAYLOAD_MAX - 1] = '\0';
    g_retryHead = (g_retryHead + 1) % RETRY_BUF_SIZE;
    if (g_retryCount < RETRY_BUF_SIZE) {
        g_retryCount++;
    } else {
        Serial.println("[MQTT] Retry buffer full — oldest sample dropped");
    }
}

static void retryFlush() {
    while (g_retryCount > 0) {
        int tail = ((g_retryHead - g_retryCount) % RETRY_BUF_SIZE
                   + RETRY_BUF_SIZE) % RETRY_BUF_SIZE;
        if (!g_mqtt.publish(g_topic.c_str(), g_retryBuf[tail])) {
            Serial.println("[MQTT] Retry flush failed — will retry later");
            return;
        }
        g_retryCount--;
        Serial.printf("[MQTT] Flushed retry (%d remaining)\n", g_retryCount);
    }
}

void MqttManager::init(const String& server, int port,
                       const String& deviceID, const String& tenantID,
                       const String& mqttUser, const String& mqttPass) {
    g_server   = server;
    g_port     = port;
    g_deviceID = deviceID;
    g_tenantID = tenantID;
    g_mqttUser = mqttUser;
    g_mqttPass = mqttPass;
    g_topic    = tenantID + "/devices/" + deviceID + "/telemetry";

#if MQTT_SKIP_CERT_VERIFY
    g_secureClient.setInsecure();
    Serial.println("[MQTT] WARNING: TLS certificate verification DISABLED");
#else
    g_secureClient.setCACert(MQTT_ROOT_CA);
#endif

    g_mqtt.setServer(server.c_str(), port);
    g_mqtt.setBufferSize(MQTT_PACKET_SIZE);
    g_mqtt.setKeepAlive(MQTT_KEEPALIVE_S);
    g_mqtt.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);

    Serial.printf("[MQTT] Topic: %s\n", g_topic.c_str());
}

void MqttManager::tick() {
    if (g_mqtt.connected()) {
        g_mqtt.loop();
        LedManager::setMqtt(LedMode::SOLID_ON);
        return;
    }

    LedManager::setMqtt(LedMode::BLINK_SLOW);

    if (g_mqttUser.isEmpty() || g_mqttPass.isEmpty()) {
        static unsigned long g_lastCfgWarnMs = 0;
        if (millis() - g_lastCfgWarnMs > 30000UL) {
            g_lastCfgWarnMs = millis();
            Serial.println("[MQTT] No credentials configured — set via web portal");
        }
        return;
    }

    if (millis() - g_lastAttemptMs < g_backoffDelayMs) return;
    g_lastAttemptMs = millis();

    String clientId = "shivex-" + g_deviceID;
    if (g_mqtt.connect(clientId.c_str(),
                       g_mqttUser.c_str(), g_mqttPass.c_str())) {
        Serial.println("[MQTT] Connected (TLS + auth)");
        LedManager::setMqtt(LedMode::SOLID_ON);
        g_backoffDelayMs = MQTT_BACKOFF_MIN_MS;
        retryFlush();
    } else {
        unsigned long jitter = (unsigned long)random(0, 500);
        g_backoffDelayMs = min(g_backoffDelayMs * 2 + jitter,
                               MQTT_BACKOFF_MAX_MS);
        Serial.printf("[MQTT] Connect failed rc=%d — retry in %lums\n",
                      g_mqtt.state(), g_backoffDelayMs);
    }
}

bool MqttManager::publish(const char* payload, size_t len) {
    if (!g_mqtt.connected()) {
        retryPush(payload);
        return false;
    }
    bool ok = g_mqtt.publish(g_topic.c_str(), payload, false);
    if (!ok) retryPush(payload);
    return ok;
}

bool MqttManager::isConnected() { return g_mqtt.connected(); }
