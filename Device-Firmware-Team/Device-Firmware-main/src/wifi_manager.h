/**
 * wifi_manager.h — Non-blocking WiFi FSM.
 * States: IDLE → CONNECTING → CONNECTED → FAILED → AP_MODE
 * No while() loops. No delay(). tick() returns immediately.
 */
#pragma once
#include <Arduino.h>

enum class WifiState {
    IDLE,
    CONNECTING,
    CONNECTED,
    FAILED,
    AP_MODE
};

namespace WifiManager {
    void      init(const String& ssid, const String& pass);
    void      tick();
    WifiState state();
    bool      isConnected();
    bool      isAPMode();
    void      startAP();
    void      stopAP();
}
