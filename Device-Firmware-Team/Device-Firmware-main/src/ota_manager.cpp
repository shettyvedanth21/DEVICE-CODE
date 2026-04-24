#include "ota_manager.h"
#include "config.h"
#include "watchdog.h"
#include "secrets.h"
#include <ArduinoOTA.h>

void OtaManager::init(const String& deviceID) {
    ArduinoOTA.setHostname(("shivex-" + deviceID).c_str());
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.printf("[OTA] Start: %s\n", type.c_str());
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Watchdog::feed();   // CRITICAL — prevents reset during long flash
        Serial.printf("[OTA] Progress: %u%%\n", progress * 100 / total);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("[OTA] Complete — rebooting");
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        if      (error == OTA_AUTH_ERROR)    Serial.println("Auth failed");
        else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive failed");
        else if (error == OTA_END_ERROR)     Serial.println("End failed");
    });

    ArduinoOTA.begin();
    Serial.printf("[OTA] Ready — hostname: shivex-%s\n", deviceID.c_str());
}

void OtaManager::tick() {
    ArduinoOTA.handle();
}
