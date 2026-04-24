/**
 * meter_data.h — MeterReading struct
 * Single source of truth for all meter values.
 * Replaces 16 separate global floats in v1.
 */
#pragma once
#include <stdint.h>

struct MeterReading {
    // Phase-neutral voltages (V)
    float v1        = 0.0f;
    float v2        = 0.0f;
    float v3        = 0.0f;
    float vAvg      = 0.0f;   // average phase-neutral
    float vLine     = 0.0f;   // line-to-line

    // Phase currents (A)
    float i1        = 0.0f;
    float i2        = 0.0f;
    float i3        = 0.0f;
    float iAvg      = 0.0f;

    // Power & energy
    float powerW    = 0.0f;   // active power in Watts (v1: was kW * 1000)
    float energyKwh = 0.0f;
    float pf        = 0.0f;   // power factor 0.0–1.0
    float freqHz    = 0.0f;
    float kvar      = 0.0f;
    float kvah      = 0.0f;
    float runHours  = 0.0f;

    // Diagnostics
    uint16_t modbusErrors = 0;
    uint32_t meterSerial  = 0;
};

// One global instance — written by modbus_reader, read by telemetry
// volatile ensures compiler doesn't cache across module boundaries
extern volatile MeterReading g_meter;
