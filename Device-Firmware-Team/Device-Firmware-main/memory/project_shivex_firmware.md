---
name: Shivex firmware v2 architecture
description: Shivex.ai ESP32+MFM384 Modbus-to-MQTT firmware — key decisions, module structure, and technical constraints
type: project
---

ESP32 gateway reads MFM384 energy meter via Modbus RTU, publishes 26-field JSON telemetry (schema v2) to MQTT over WiFi. Fully rewritten from 871-line monolith into 11 modules, each under 250 lines.

**Why:** v1 had 7 blocking `delay()` calls in `loop()`, no watchdog, hardcoded passwords in source, 16 global floats instead of a struct, `String[5]` retry buffer causing heap fragmentation.

**How to apply:** Any future changes should respect the non-blocking constraint (loop() < 50ms), all delays in src/*.cpp will break CI grep check.

## File structure
- `src/config.h` — all constants, NO Arduino.h (must compile on native for tests)
- `src/meter_data.h` — `MeterReading` struct, `extern volatile MeterReading g_meter`
- `src/modbus_reader.cpp` — reads one register pair per tick(); meter serial deferred 500ms
- `src/telemetry.cpp` — owns all JSON; takes wifiRssi+uptimeS as params (no platform deps)
- `src/web_portal.cpp` — PROGMEM HTML, deferred restart, never returns WiFi password
- `include/secrets.h` — gitignored; defines ADMIN_PASSWORD, AP_PASSWORD, OTA_PASSWORD
- `test/test_*/` — PlatformIO requires one subdirectory per test suite for native env

## Key constraints
- MFM384 register addresses are verified against hardware — do not change (REG_VN1=0 through REG_RUNHOURS=82, REG_METER_SERIAL=0x02AC)
- MQTT QoS 0, retain=false — backend designed for this
- ArduinoJson v6 (NOT v7) — backend parser expects v6 output format
- `ARDUINOJSON_USE_DOUBLE=0` in native env — matches Arduino float precision, keeps payload under 640 bytes
- platformio.ini three envs: esp32dev (USB), ota (field OTA via mDNS), native (unit tests)

## Test status (as of 2026-04-21)
- `pio test -e native` — 16/16 tests pass (test_modbus: 6, test_retry_buf: 4, test_telemetry: 6)
- `pio run -e esp32dev` — compiles successfully, 64.3% flash, 16.8% RAM
