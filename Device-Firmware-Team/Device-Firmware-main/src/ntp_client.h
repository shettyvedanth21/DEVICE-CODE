/**
 * ntp_client.h — Non-blocking NTP sync.
 * setup() calls configTime(); tick() polls for validity.
 * Never blocks. Telemetry checks isReady() before publishing.
 */
#pragma once
#include <Arduino.h>

namespace NtpClient {
    void init();            // call after WiFi connects
    void tick();            // non-blocking readiness check
    bool isReady();         // true once time() > NTP_VALID_EPOCH
    String getISOTimeUTC(); // returns "" if not ready
}
