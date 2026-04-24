#include "wifi_manager.h"
#include "config.h"
#include "led_manager.h"
#include "ntp_client.h"
#include "secrets.h"
#include <WiFi.h>

static WifiState g_state         = WifiState::IDLE;
static String    g_ssid;
static String    g_pass;
static unsigned long g_connectStart = 0;

void WifiManager::init(const String& ssid, const String& pass) {
    g_ssid = ssid;
    g_pass = pass;
    WiFi.setAutoReconnect(false); // FSM handles reconnect
    WiFi.persistent(false);

    if (ssid.isEmpty()) {
        startAP();
    } else {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
        g_connectStart = millis();
        g_state = WifiState::CONNECTING;
        LedManager::setWifi(LedMode::BLINK_FAST);
    }
}

void WifiManager::tick() {
    switch (g_state) {
        case WifiState::CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                g_state = WifiState::CONNECTED;
                LedManager::setWifi(LedMode::SOLID_ON);
                NtpClient::init();
                Serial.printf("[WiFi] Connected — IP: %s\n",
                    WiFi.localIP().toString().c_str());
            } else if (millis() - g_connectStart > WIFI_CONNECT_TIMEOUT_MS) {
                g_state = WifiState::FAILED;
                Serial.println("[WiFi] Connect timeout — starting AP");
                startAP();
            }
            break;

        case WifiState::CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WiFi] Lost connection — reconnecting...");
                g_state = WifiState::CONNECTING;
                g_connectStart = millis();
                LedManager::setWifi(LedMode::BLINK_FAST);
                WiFi.begin(g_ssid.c_str(), g_pass.c_str());
            }
            break;

        case WifiState::AP_MODE:
        case WifiState::FAILED:
        case WifiState::IDLE:
            break;
    }
}

bool WifiManager::isConnected() { return g_state == WifiState::CONNECTED; }
bool WifiManager::isAPMode()    { return g_state == WifiState::AP_MODE;   }
WifiState WifiManager::state()  { return g_state; }

void WifiManager::startAP() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    g_state = WifiState::AP_MODE;
    LedManager::setWifi(LedMode::AP_ALTERNATE);
    Serial.printf("[WiFi] AP started — IP: %s\n",
        WiFi.softAPIP().toString().c_str());
}

void WifiManager::stopAP() {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    g_state = WifiState::IDLE;
}
