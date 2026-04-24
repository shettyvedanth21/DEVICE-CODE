/**
 * telemetry.h — Schema v2 payload builder.
 * Owns all JSON serialisation. No other module touches ArduinoJson.
 * No Arduino.h dependency — compiles on native for unit testing.
 *
 * Payload conforms to backend validation:
 *   - Top-level reserved fields: device_id, tenant_id, timestamp,
 *     schema_version, device_metadata
 *   - All other top-level fields must be numeric telemetry values
 *   - Non-numeric metadata (firmware_version, boot_reason, meter_serial)
 *     placed inside device_metadata object
 */
#pragma once
#include "meter_data.h"
#include <stddef.h>
#include <stdint.h>

namespace Telemetry {
    /**
     * Build a schema v2 MQTT payload into buf.
     * Returns actual byte count written, or 0 on failure (overflow or buf too small).
     * buf must be at least MQTT_PAYLOAD_MAX bytes.
     *
     * wifiRssi and uptimeS are passed in by the caller (from WiFi.RSSI() and
     * millis()/1000) so this function has zero platform dependencies and is
     * testable on the native environment without Arduino stubs.
     */
    size_t buildPayload(
        const MeterReading& m,
        const char*  deviceID,
        const char*  tenantID,
        const char*  timestamp,
        const char*  bootReason,
        int          wifiRssi,
        uint32_t     uptimeS,
        char*        buf,
        size_t       bufLen
    );
}
