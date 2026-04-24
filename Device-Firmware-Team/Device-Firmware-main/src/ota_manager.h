/**
 * ota_manager.h — ArduinoOTA with password auth and watchdog feed during flash.
 * Watchdog feed in onProgress is critical — flash takes >30s on large firmware.
 */
#pragma once
#include <Arduino.h>

namespace OtaManager {
    void init(const String& deviceID);
    void tick();   // call every loop() — ArduinoOTA.handle()
}
