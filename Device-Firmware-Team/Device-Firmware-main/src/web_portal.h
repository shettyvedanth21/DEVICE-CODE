/**
 * web_portal.h — SoftAP configuration portal.
 * HTML stored in PROGMEM flash. No String heap construction on each request.
 * Password NEVER returned to browser. All inputs validated before NVS write.
 */
#pragma once
#include <WebServer.h>
#include "storage.h"

namespace WebPortal {
    // pendingRestart is set true by handleSaveAll — main loop checks and restarts
    extern volatile bool pendingRestart;
    extern volatile unsigned long restartFlagTime;

    void init(DeviceConfig& cfg);
    void tick();   // server.handleClient()
}
