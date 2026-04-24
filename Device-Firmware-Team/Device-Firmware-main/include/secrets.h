/**
 * secrets.h — local development copy.
 * NEVER commit this file — it is in .gitignore.
 * For production: fill MQTT_ROOT_CA with a real broker CA certificate.
 * For local dev: build with -DMQTT_SKIP_CERT_VERIFY=1 in platformio.ini.
 */
#pragma once

#define ADMIN_PASSWORD   "change-me-admin"
#define AP_PASSWORD      "change-me-ap"
#define OTA_PASSWORD     "change-me-ota"

// MQTT_ROOT_CA must be defined for production TLS (port 8883).
// Build with -DMQTT_SKIP_CERT_VERIFY=1 to skip during development.
#define MQTT_ROOT_CA     \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIDHzCCAgegAwIBAgIUJX6RLAztTWsVboRRQPx9f7HkeLAwDQYJKoZIhvcNAQEL\n" \
  "BQAwHzEdMBsGA1UEAwwUU2hpdmV4IExvY2FsIE1RVFQgQ0EwHhcNMjYwNDIzMDg0\n" \
  "NjU1WhcNMzYwNDIwMDg0NjU1WjAfMR0wGwYDVQQDDBRTaGl2ZXggTG9jYWwgTVFU\n" \
  "VCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALgIXnYgsmKE8td7\n" \
  "9it3YNgx1l8vIJ7gNRQohfO9rINHjcIuEQcReSD043wYEAhBzuCy5hmqJBEtMy6s\n" \
  "HK8gOnVbPbeabYhfLeVH9VRJWB1vOjMfa+mWKYnQTIIb1PbnWyUklbbB5b7LTSLM\n" \
  "zCwPHcPSo0D2G1yACvQrslRVVIJ0pmBan42ec8vwlWF4bKRZHZQAiKCbhYQ4PlFY\n" \
  "npsYjcHQ2i2bZViulQNs+oCBMj2ZByroYbH7wiQjpDsS3se/q9WVvBcc/tu10KDw\n" \
  "pjqZ6sagAVsmsWlIGTftM+96jq2QuyVVXK/w3J9bkBN/KXHlo3kW8mG9BiMYHD7f\n" \
  "vBV5fVsCAwEAAaNTMFEwHQYDVR0OBBYEFPuv1gK3qadhE6PjSe+A9uNEEsB6MB8G\n" \
  "A1UdIwQYMBaAFPuv1gK3qadhE6PjSe+A9uNEEsB6MA8GA1UdEwEB/wQFMAMBAf8w\n" \
  "DQYJKoZIhvcNAQELBQADggEBAEqul5LHoQcAKplStlDOWCJRea83qKmrc75Kh7FV\n" \
  "BZZ3NhFtGmNddXCk/fjRJoQxzySJ2LkNdw3bW0GW8Y+m/QLKXS1yToflaDh8aXq3\n" \
  "BVN2nlLK4z20D70eYkqL7j5tO9oUjQ6H+h1THHF7cDyU2G3X10DtAeenlc3Ux8QA\n" \
  "WGwJZOZVyoz+VtijZi2pfRCQJvWsokPZvSpFhgKZPgPqAre3NDvd3DpI7WsDCdUE\n" \
  "W5kVe3oNU4Ls2EpJC8rj3o4AGq7Xwkqcgkz3atN1JrmuNqBuC2juSfrivq96iiJt\n" \
  "0Orz04R4Skk+nZbU56Qvlvuhy/RXPBded7j+UuyqJXKE/N0=\n" \
  "-----END CERTIFICATE-----\n"
