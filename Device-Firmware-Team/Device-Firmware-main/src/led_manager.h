/**
 * led_manager.h — All LED state machines in one place.
 * No other module writes to LED pins directly.
 */
#pragma once

enum class LedMode {
    OFF,
    SOLID_ON,
    BLINK_SLOW,   // 1000ms period — MQTT backoff
    BLINK_FAST,   // 150ms period — MQTT connecting
    AP_ALTERNATE  // WiFi/MQTT alternate blink — SoftAP mode
};

namespace LedManager {
    void init();
    void tick();   // call every loop() — non-blocking, millis()-gated

    void setWifi(LedMode mode);
    void setMqtt(LedMode mode);
}
