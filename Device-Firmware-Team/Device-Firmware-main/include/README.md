# Shivex.ai Device Firmware v2.0

ESP32 + MFM384 energy meter gateway. Reads power data over Modbus RTU,
publishes to MQTT broker as JSON telemetry. Configurable via SoftAP web portal.

## First-time setup

```bash
# 1. Clone repo
git clone <repo-url>
cd shivex_firmware

# 2. Create secrets file (NEVER commit this)
cp secrets.h.template include/secrets.h
# Edit include/secrets.h — set ADMIN_PASSWORD, AP_PASSWORD, OTA_PASSWORD

# 3. Build and flash via USB
pio run -e esp32dev --target upload

# 4. Monitor serial output
pio device monitor -e esp32dev
```

## OTA update (field deployment)

```bash
export OTA_PASSWORD="your-ota-password"
pio run -e ota --target upload
```

## Run unit tests (no hardware needed)

```bash
pio test -e native
```

## MQTT Payload — Schema v2

| Field            | Type   | Unit   | Notes                        |
|------------------|--------|--------|------------------------------|
| schema_version   | string | —      | Always "v2"                  |
| device_id        | string | —      | From NVS config              |
| org_id           | string | —      | From NVS config              |
| firmware_version | string | —      | Injected at build time       |
| timestamp        | string | ISO8601| UTC from NTP                 |
| meter_serial     | uint32 | —      | Read from MFM384 reg 0x02AC  |
| uptime_s         | uint32 | s      | millis()/1000                |
| wifi_rssi        | int8   | dBm    | WiFi.RSSI()                  |
| boot_reason      | string | —      | esp_reset_reason()           |
| modbus_errors    | uint16 | —      | Cumulative since boot        |
| voltage_l1/l2/l3 | float | V      | Phase-neutral voltages       |
| voltage_avg      | float  | V      | Average phase-neutral        |
| voltage_line     | float  | V      | Line-to-line voltage         |
| current_l1/l2/l3 | float | A      | Phase currents               |
| current_avg      | float  | A      | Average phase current        |
| power_w          | float  | W      | Active power                 |
| energy_kwh       | float  | kWh    | Cumulative energy            |
| power_factor     | float  | —      | 0.0–1.0                      |
| frequency_hz     | float  | Hz     | Grid frequency               |
| kvar             | float  | kVAR   | Reactive power               |
| kvah             | float  | kVAh   | Apparent energy              |
| run_hours        | float  | h      | Meter run hours              |

## Hardware pinout

| Signal   | ESP32 GPIO |
|----------|-----------|
| RS485 RX | 16        |
| RS485 TX | 17        |
| DE/RE    | 4         |
| WiFi LED | 2         |
| MQTT LED | 5         |
| Setup Btn| 25        |
