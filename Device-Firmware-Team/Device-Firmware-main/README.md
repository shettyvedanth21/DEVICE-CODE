# Shivex.ai Device Firmware v2.0

ESP32 + MFM384 three-phase energy meter gateway. Reads power data over Modbus RTU,
publishes to MQTT broker as JSON telemetry (schema v2). Configurable via SoftAP
web portal with no laptop needed.

## First-time setup

```bash
# 1. Clone repository
git clone <repo-url>
cd shivex_firmware

# 2. Create secrets file (NEVER commit this file — it is in .gitignore)
cp secrets.h.template include/secrets.h
# Edit include/secrets.h — set ADMIN_PASSWORD, AP_PASSWORD, OTA_PASSWORD

# 3. Build and flash via USB
pio run -e esp32dev --target upload

# 4. Monitor serial output
pio device monitor -e esp32dev
```

## OTA update (field deployment, no USB required)

```bash
export OTA_PASSWORD="your-ota-password"
pio run -e ota --target upload
```

The device hostname is `shivex-<deviceID>.local` (mDNS). Update `upload_port`
in `platformio.ini [env:ota]` if your network doesn't support mDNS.

## Run unit tests (no hardware needed)

```bash
pio test -e native
```

All three test suites must pass: `test_modbus`, `test_retry_buf`, `test_telemetry`.

## Web portal

1. On first boot with no WiFi configured, device starts SoftAP `Shivex_Setup`
2. Connect to the AP and browse to `http://192.168.4.1`
3. Enter WiFi credentials and MQTT settings
4. Click **Save & Restart** — device reboots into STA mode

## MQTT Payload — Schema v2

Topic: `{orgID}/devices/{deviceID}/telemetry`

| Field            | Type    | Unit   | Notes                              |
|------------------|---------|--------|------------------------------------|
| schema_version   | string  | —      | Always `"v2"`                      |
| device_id        | string  | —      | From NVS config                    |
| org_id           | string  | —      | From NVS config                    |
| firmware_version | string  | —      | Injected at build time             |
| timestamp        | string  | ISO8601| UTC from NTP                       |
| meter_serial     | uint32  | —      | Read from MFM384 register 0x02AC   |
| uptime_s         | uint32  | s      | `millis()/1000`                    |
| wifi_rssi        | int8    | dBm    | `WiFi.RSSI()` at publish time      |
| boot_reason      | string  | —      | `esp_reset_reason()` on boot       |
| modbus_errors    | uint16  | —      | Cumulative failed reads since boot |
| voltage_l1       | float   | V      | Phase 1 voltage                    |
| voltage_l2       | float   | V      | Phase 2 voltage                    |
| voltage_l3       | float   | V      | Phase 3 voltage                    |
| voltage_avg      | float   | V      | Average phase-neutral voltage      |
| voltage_line     | float   | V      | Line-to-line voltage               |
| current_l1       | float   | A      | Phase 1 current                    |
| current_l2       | float   | A      | Phase 2 current                    |
| current_l3       | float   | A      | Phase 3 current                    |
| current_avg      | float   | A      | Average phase current              |
| power_w          | float   | W      | Active power                       |
| energy_kwh       | float   | kWh    | Cumulative energy                  |
| power_factor     | float   | —      | 0.0–1.0                            |
| frequency_hz     | float   | Hz     | Grid frequency                     |
| kvar             | float   | kVAR   | Reactive power                     |
| kvah             | float   | kVAh   | Apparent energy                    |
| run_hours        | float   | h      | Meter run hours                    |

## Hardware pinout

| Signal      | ESP32 GPIO |
|-------------|------------|
| RS485 RX    | 16         |
| RS485 TX    | 17         |
| RS485 DE/RE | 4          |
| WiFi LED    | 2          |
| MQTT LED    | 5          |
| Setup button| 25         |

## Secrets

Three secrets live in `include/secrets.h` (gitignored):

| Constant       | Purpose                                              |
|----------------|------------------------------------------------------|
| ADMIN_PASSWORD | Web portal MQTT settings unlock                      |
| AP_PASSWORD    | SoftAP WPA2 password                                 |
| OTA_PASSWORD   | ArduinoOTA auth — matches `--auth=` in `[env:ota]`   |
| MQTT_ROOT_CA   | Broker TLS root CA cert (PEM) — required for production |

## Local MQTT testing

For testing against a local broker (e.g. your Mac) whose certificate does not match
the connection IP, use the `localdev` build environment. This disables TLS certificate
verification while keeping the TLS connection and MQTT username/password authentication.

**This mode must NEVER be used in production.**

```bash
# 1. Build and flash the localdev firmware
pio run -e localdev --target upload

# 2. Monitor serial — you will see this banner at boot:
#    !!! TLS VERIFY: DISABLED (localdev build — not for production) !!!
pio device monitor -e localdev

# 3. Connect to the ESP SoftAP (Shivex_Setup) and open http://192.168.4.1

# 4. In the web portal:
#    - Enter WiFi credentials (same network as your Mac)
#    - Unlock MQTT settings with the admin password
#    - Set Broker to your Mac's local IP (e.g. 192.168.1.50)
#    - Set Port to 8883
#    - Set MQTT Username and Password as configured on your broker
#    - Click Save & Restart

# 5. Device connects to your local broker over TLS (cert verify skipped)
```

Key points:
- The `localdev` env sets `MQTT_SKIP_CERT_VERIFY=1` and enables verbose debug logs
- TLS is still used (port 8883, `WiFiClientSecure`), only the cert check is skipped
- MQTT username/password authentication is still required
- The production `esp32dev` and `ota` environments always enforce full cert verification

## CI

GitHub Actions runs on every push/PR to `main`:
- `pio run -e esp32dev` — full compile check
- `pio test -e native` — all unit tests

On git tag `v*.*.*`, a release binary is built and uploaded to GitHub Releases
as `shivex-firmware-<version>.bin`.
