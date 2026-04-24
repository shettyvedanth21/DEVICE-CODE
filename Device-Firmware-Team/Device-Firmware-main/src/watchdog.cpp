#include "watchdog.h"
#include "config.h"
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_idf_version.h>
#include <rom/rtc.h>

void Watchdog::init() {
    esp_err_t err = ESP_OK;
#if ESP_IDF_VERSION_MAJOR >= 5
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms    = (uint32_t)WATCHDOG_TIMEOUT_S * 1000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic  = true
    };
    err = esp_task_wdt_init(&twdt_config);
    if (err == ESP_ERR_INVALID_STATE) {
        err = esp_task_wdt_reconfigure(&twdt_config);
    }
#else
    err = esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);
#endif
    if (err != ESP_OK) {
        Serial.printf("[Watchdog] Init/reconfigure failed err=%d\n", (int)err);
        return;
    }

    esp_err_t addErr = esp_task_wdt_add(NULL);
    if (addErr == ESP_ERR_INVALID_STATE) {
        Serial.println("[Watchdog] Loop task already registered with TWDT");
    } else if (addErr != ESP_OK) {
        Serial.printf("[Watchdog] Add current task failed err=%d\n", (int)addErr);
    }
}

void Watchdog::feed() {
    esp_task_wdt_reset();
}

const char* Watchdog::bootReasonStr() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "POWER_ON";
        case ESP_RST_EXT:       return "EXTERNAL_RESET";
        case ESP_RST_SW:        return "SOFTWARE_RESET";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INT_WATCHDOG";
        case ESP_RST_TASK_WDT:  return "TASK_WATCHDOG";
        case ESP_RST_WDT:       return "OTHER_WATCHDOG";
        case ESP_RST_DEEPSLEEP: return "DEEP_SLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        default:                return "UNKNOWN";
    }
}
