#include "ntp_client.h"
#include "config.h"
#include <time.h>

static bool g_ready = false;

void NtpClient::init() {
    g_ready = false;
    configTime(NTP_GMT_OFFSET_S, NTP_DAYLIGHT_S, NTP_SERVER);
}

void NtpClient::tick() {
    if (!g_ready && time(nullptr) > NTP_VALID_EPOCH) {
        g_ready = true;
        Serial.println("[NTP] Time synced");
    }
}

bool NtpClient::isReady() { return g_ready; }

String NtpClient::getISOTimeUTC() {
    if (!g_ready) return "";
    time_t now = time(nullptr);
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
    return String(buf);
}
