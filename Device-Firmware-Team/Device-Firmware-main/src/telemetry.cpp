#include "telemetry.h"
#include "config.h"
#include <ArduinoJson.h>

#if ARDUINOJSON_VERSION_MAJOR >= 7
static JsonDocument doc;
#else
static const size_t JSON_DOC_SIZE = 768;
#endif

size_t Telemetry::buildPayload(
    const MeterReading& m,
    const char*  deviceID,
    const char*  tenantID,
    const char*  timestamp,
    const char*  bootReason,
    int          wifiRssi,
    uint32_t     uptimeS,
    char*        buf,
    size_t       bufLen)
{
#if ARDUINOJSON_VERSION_MAJOR >= 7
    doc.clear();
#else
    DynamicJsonDocument doc(JSON_DOC_SIZE);
#endif

    doc["schema_version"]   = "v2";
    doc["device_id"]        = deviceID;
    doc["tenant_id"]        = tenantID;
    doc["timestamp"]        = timestamp;

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonObject meta = doc["device_metadata"].to<JsonObject>();
#else
    JsonObject meta = doc.createNestedObject("device_metadata");
#endif
    meta["firmware_version"] = FIRMWARE_VERSION;
    meta["boot_reason"]      = bootReason;
    meta["meter_serial"]     = m.meterSerial;

    // ── Numeric diagnostic fields ───────────────────────────────────────
    doc["uptime_s"]         = uptimeS;
    doc["wifi_rssi"]        = wifiRssi;
    doc["modbus_errors"]    = m.modbusErrors;

    // ── Voltage (5 fields) ───────────────────────────────────────────────
    doc["voltage_l1"]       = m.v1;
    doc["voltage_l2"]       = m.v2;
    doc["voltage_l3"]       = m.v3;
    doc["voltage_avg"]      = m.vAvg;
    doc["voltage_line"]     = m.vLine;

    // ── Current (4 fields) ───────────────────────────────────────────────
    doc["current_l1"]       = m.i1;
    doc["current_l2"]       = m.i2;
    doc["current_l3"]       = m.i3;
    doc["current_avg"]      = m.iAvg;

    // ── Power & energy (7 fields) ────────────────────────────────────────
    doc["power_w"]          = m.powerW;
    doc["energy_kwh"]       = m.energyKwh;
    doc["power_factor"]     = m.pf;
    doc["frequency_hz"]     = m.freqHz;
    doc["kvar"]             = m.kvar;
    doc["kvah"]             = m.kvah;
    doc["run_hours"]        = m.runHours;

#if ARDUINOJSON_VERSION_MAJOR >= 7
    if (doc.memoryUsage() == 0) {
#else
    if (doc.overflowed()) {
#endif
        return 0;
    }

    size_t len = serializeJson(doc, buf, bufLen);
    if (len == 0 || len >= bufLen) {
        return 0;
    }
    return len;
}
