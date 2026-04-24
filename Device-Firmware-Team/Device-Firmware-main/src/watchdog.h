/**
 * watchdog.h — Hardware watchdog via esp_task_wdt.
 * Must be fed every loop(). If loop() blocks >WATCHDOG_TIMEOUT_S, device resets.
 */
#pragma once

namespace Watchdog {
    void init();   // call once in setup()
    void feed();   // call at top of every loop() iteration
    const char* bootReasonStr();  // human-readable reset reason for telemetry
}
