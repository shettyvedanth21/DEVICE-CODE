#include "web_portal.h"
#include "config.h"
#include "storage.h"
#include "secrets.h"
#include "qr_provision.h"
#include <ArduinoJson.h>
#include <Arduino.h>

static WebServer     g_server(80);
static DeviceConfig* g_cfg = nullptr;

volatile bool          WebPortal::pendingRestart   = false;
volatile unsigned long WebPortal::restartFlagTime  = 0;

// ── Portal HTML in flash — no heap allocation on each page request ────────
// %%SSID%% %%BROKER_MASKED%% %%PORT_MASKED%% %%DEVICE_MASKED%% are replaced
// at serve-time with real values (small template substitution).
static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Shivex Setup</title>
<style>
body{font-family:system-ui,sans-serif;background:#f0f4f8;margin:0;padding:16px}
.card{background:#fff;padding:24px;border-radius:12px;max-width:400px;margin:auto;box-shadow:0 4px 12px rgba(0,0,0,.1)}
.brand{font-size:18px;font-weight:700;color:#0a2540;text-align:center;margin-bottom:20px}
h3{font-size:14px;border-bottom:1px solid #eee;padding-bottom:8px;color:#333;margin:0 0 12px}
label{font-size:12px;color:#666;display:block;margin-bottom:4px}
input{width:100%;padding:9px 10px;margin-bottom:12px;border:1px solid #ccc;border-radius:6px;box-sizing:border-box;font-size:14px}
input[type=file]{font-size:12px;padding:6px 10px}
textarea{width:100%;padding:9px 10px;margin-bottom:12px;border:1px solid #ccc;border-radius:6px;box-sizing:border-box;font-size:12px;font-family:monospace;resize:vertical}
.btn{width:100%;padding:11px;background:#0a2540;border:none;color:#fff;border-radius:6px;font-size:14px;cursor:pointer;margin-bottom:8px}
.btn:hover{background:#163a5e}
.btn-sec{background:#6c757d}
.info{background:#f8f9fa;padding:9px 12px;border-radius:6px;font-size:12px;margin-bottom:12px}
.err{color:red;font-size:12px;display:none;text-align:center}
.qr-ok{background:#d4edda;color:#155724;padding:8px 12px;border-radius:6px;font-size:12px;margin-top:8px}
.qr-err{background:#f8d7da;color:#721c24;padding:8px 12px;border-radius:6px;font-size:12px;margin-top:8px}
.footer{text-align:center;font-size:11px;color:#aaa;margin-top:16px}
hr{border:none;border-top:1px solid #eee;margin:16px 0}
#qr-video{width:100%;border-radius:6px;display:none;margin-bottom:8px}
#qr-status{display:none}
</style>
</head>
<body>
<div class="card">
<div class="brand">Shivex.ai</div>
<h3>WiFi</h3>
<div class="info">Current SSID: <b>%%SSID%%</b></div>
<div id="wifi-edit" style="display:none">
  <label>SSID</label><input type="text" id="w-ssid">
  <label>Password</label><input type="password" id="w-pass">
  <button class="btn btn-sec" onclick="cancelWifi()">Cancel</button>
</div>
<button class="btn" id="wifi-btn" onclick="editWifi()">Edit WiFi</button>
<hr>
<h3>MQTT Settings</h3>
<div class="info" id="mqtt-status">Status: Locked</div>
<div id="mqtt-auth">
  <label>Admin Password</label>
  <input type="password" id="admin-pwd">
  <button class="btn" onclick="unlockMqtt()">Unlock</button>
  <div class="err" id="auth-err">Wrong password</div>
</div>
<div id="mqtt-fields" style="display:none">
  <label>Broker</label><input type="text" id="m-server" value="%%BROKER_MASKED%%">
  <label>Port</label><input type="number" id="m-port" value="%%PORT_MASKED%%">
  <label>Device ID</label><input type="text" id="m-device" value="%%DEVICE_MASKED%%">
  <label>Tenant ID</label><input type="text" id="m-tenant" value="">
  <label>MQTT Username</label><input type="text" id="m-user" value="">
  <label>MQTT Password</label><input type="password" id="m-pass" placeholder="Enter new password">
</div>
<div id="qr-section" style="display:none">
  <hr>
  <h3>QR Provisioning</h3>
  <div class="info">Import the provisioning JSON payload from a QR scan app or file. Camera scan may not work over HTTP — use JSON import for guaranteed provisioning.</div>
  <label>Import JSON File</label>
  <input type="file" accept=".json,application/json" onchange="importJsonFile(event)">
  <label>Paste JSON Payload</label>
  <textarea id="qr-paste" rows="4" placeholder='{"v":1,"tenant_id":"...","device_id":"...","mqtt_host":"...","mqtt_port":8883,"mqtt_username":"...","mqtt_password":"..."}'></textarea>
  <button class="btn" onclick="applyPastedJson()">Apply Provisioning Payload</button>
  <div id="qr-camera-section">
    <hr>
    <div class="info">Camera/QR image scan requires a secure browser context (HTTPS). Not available when connected to this device's HTTP hotspot.</div>
    <button class="btn btn-sec" id="qr-scan-btn" onclick="startQrCamera()">Scan QR (Camera)</button>
    <video id="qr-video"></video>
    <label>Upload QR Image</label>
    <input type="file" accept="image/*" onchange="scanQrImage(event)">
  </div>
  <div id="qr-status"></div>
</div>
<hr>
<button class="btn" onclick="saveAll()">Save &amp; Restart</button>
<div class="footer">Shivex Firmware v%%FW_VER%%</div>
</div>
<script>
var mqttOk=false,pendingQr=null,qrStream=null,qrTimer=null;
var REQ_FIELDS=['tenant_id','device_id','mqtt_host','mqtt_port','mqtt_username','mqtt_password'];

function editWifi(){
  document.getElementById('wifi-edit').style.display='block';
  document.getElementById('wifi-btn').style.display='none';
}
function cancelWifi(){
  document.getElementById('wifi-edit').style.display='none';
  document.getElementById('wifi-btn').style.display='block';
}

function showQrMsg(m,isErr){
  var s=document.getElementById('qr-status');
  s.style.display='block';
  s.className=isErr?'qr-err':'qr-ok';
  s.textContent=m;
}

function validateQrPayload(d){
  for(var i=0;i<REQ_FIELDS.length;i++){
    if(!d.hasOwnProperty(REQ_FIELDS[i])||!d[REQ_FIELDS[i]]&&d[REQ_FIELDS[i]]!==0)
      return 'Missing required field: '+REQ_FIELDS[i];
  }
  if(!d.mqtt_password)return 'Missing required field: mqtt_password';
  return '';
}

function applyQrData(d){
  document.getElementById('m-server').value=d.mqtt_host||'';
  document.getElementById('m-port').value=d.mqtt_port||'';
  document.getElementById('m-device').value=d.device_id||'';
  document.getElementById('m-tenant').value=d.tenant_id||'';
  document.getElementById('m-user').value=d.mqtt_username||'';
  document.getElementById('m-pass').value=d.mqtt_password||'';
  if(d.wifi_ssid){
    document.getElementById('w-ssid').value=d.wifi_ssid;
    document.getElementById('wifi-edit').style.display='block';
    document.getElementById('wifi-btn').style.display='none';
  }
  if(d.wifi_password)document.getElementById('w-pass').value=d.wifi_password;
  showQrMsg('Provisioning data applied \u2014 review fields and Save',false);
}

function handleQrPayload(raw){
  try{
    var d=JSON.parse(raw);
    if(d.v!==1){showQrMsg('Unsupported payload version: '+d.v,true);return;}
    var vErr=validateQrPayload(d);
    if(vErr){showQrMsg(vErr,true);return;}
    if(!mqttOk){pendingQr=d;showQrMsg('Unlock MQTT settings first \u2014 data will auto-fill after unlock.',true);return;}
    applyQrData(d);
  }catch(e){showQrMsg('Invalid JSON: '+e.message,true);}
}

function unlockMqtt(){
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/checkauth',true);
  xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
  xhr.onload=function(){
    if(xhr.status===200&&xhr.responseText==='OK'){
      mqttOk=true;
      document.getElementById('mqtt-auth').style.display='none';
      document.getElementById('mqtt-fields').style.display='block';
      document.getElementById('qr-section').style.display='block';
      document.getElementById('mqtt-status').textContent='Status: Unlocked';
      if(!window.isSecureContext)
        document.getElementById('qr-camera-section').style.display='none';
      fetch('/getmqtt').then(function(r){return r.json();}).then(function(d){
        document.getElementById('m-server').value=d.server;
        document.getElementById('m-port').value=d.port;
        document.getElementById('m-device').value=d.device;
        document.getElementById('m-tenant').value=d.tenant;
        document.getElementById('m-user').value=d.user;
        if(pendingQr){applyQrData(pendingQr);pendingQr=null;}
      });
    }else{
      document.getElementById('auth-err').style.display='block';
    }
  };
  xhr.send('pwd='+encodeURIComponent(document.getElementById('admin-pwd').value));
}

function startQrCamera(){
  if(qrStream){stopQrCamera();return;}
  if(!window.isSecureContext||!navigator.mediaDevices||!('BarcodeDetector' in window)){
    showQrMsg('Camera QR requires HTTPS. Use JSON import above instead.',true);
    return;
  }
  navigator.mediaDevices.getUserMedia({video:{facingMode:'environment'}}).then(function(stream){
    qrStream=stream;
    var v=document.getElementById('qr-video');
    v.srcObject=stream;v.style.display='block';v.play();
    document.getElementById('qr-scan-btn').textContent='Stop Scanning';
    var det=new BarcodeDetector({formats:['qr_code']});
    qrTimer=setInterval(function(){
      det.detect(v).then(function(b){if(b.length>0){stopQrCamera();handleQrPayload(b[0].rawValue);}}).catch(function(){});
    },500);
  }).catch(function(e){showQrMsg('Camera access denied or unavailable',true);});
}

function stopQrCamera(){
  if(qrTimer){clearInterval(qrTimer);qrTimer=null;}
  if(qrStream){qrStream.getTracks().forEach(function(t){t.stop();});qrStream=null;}
  var v=document.getElementById('qr-video');v.style.display='none';v.srcObject=null;
  document.getElementById('qr-scan-btn').textContent='Scan QR (Camera)';
}

function scanQrImage(e){
  var f=e.target.files[0];if(!f)return;
  if(!window.isSecureContext||!('BarcodeDetector' in window)){
    showQrMsg('QR image decode requires HTTPS. Paste JSON payload instead.',true);
    return;
  }
  var img=new Image();
  img.onload=function(){
    var det=new BarcodeDetector({formats:['qr_code']});
    det.detect(img).then(function(b){
      if(b.length>0)handleQrPayload(b[0].rawValue);
      else showQrMsg('No QR code found in image',true);
    }).catch(function(err){showQrMsg('QR decode failed: '+err.message,true);});
  };
  img.src=URL.createObjectURL(f);
}

function importJsonFile(e){
  var f=e.target.files[0];if(!f)return;
  if(f.size>1024*1024){showQrMsg('File too large (max 1 MB)',true);return;}
  var r=new FileReader();
  r.onload=function(ev){handleQrPayload(ev.target.result);};
  r.readAsText(f);
}

function applyPastedJson(){
  var t=document.getElementById('qr-paste').value.trim();
  if(!t){showQrMsg('Paste a JSON payload first',true);return;}
  handleQrPayload(t);
}

function saveAll(){
  var ssid=document.getElementById('w-ssid').value||'%%SSID%%';
  var pass=document.getElementById('w-pass').value;
  var p='ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass);
  if(mqttOk){
    p+='&server='+encodeURIComponent(document.getElementById('m-server').value);
    p+='&port='+document.getElementById('m-port').value;
    p+='&device='+encodeURIComponent(document.getElementById('m-device').value);
    p+='&tenant='+encodeURIComponent(document.getElementById('m-tenant').value);
    p+='&mqttuser='+encodeURIComponent(document.getElementById('m-user').value);
    var mp=document.getElementById('m-pass').value;
    if(mp)p+='&mqttpass='+encodeURIComponent(mp);
  }
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/saveall',true);
  xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
  xhr.onload=function(){ alert('Saved. Device restarting...'); };
  xhr.send(p);
}
</script>
</body></html>
)rawliteral";

// ── Template substitution ──────────────────────────────────────────────────
static String buildPortalPage(const DeviceConfig& cfg) {
    // Load HTML from flash into String (done once per request, ~3KB)
    String page(FPSTR(PORTAL_HTML));

    auto maskStr = [](const String& s, int visible) -> String {
        if ((int)s.length() <= visible) return s;
        String r = s.substring(0, visible);
        for (int i = 0; i < (int)s.length() - visible; i++) r += '*';
        return r;
    };

    page.replace("%%SSID%%",           cfg.wifiSSID.isEmpty() ? "(none)" : cfg.wifiSSID);
    page.replace("%%BROKER_MASKED%%",  maskStr(cfg.mqttServer, 7));
    page.replace("%%PORT_MASKED%%",    maskStr(String(cfg.mqttPort), 2));
    page.replace("%%DEVICE_MASKED%%",  maskStr(cfg.deviceID, 5));
    page.replace("%%FW_VER%%",         FIRMWARE_VERSION);
    return page;
}

// ── Input validation helpers ───────────────────────────────────────────────
static bool validPort(const String& s) {
    if (s.isEmpty()) return false;
    int p = s.toInt();
    return p >= 1 && p <= 65535;
}
static bool validDeviceID(const String& s) {
    if (s.isEmpty() || s.length() > 32) return false;
    for (char c : s) if (!isAlphaNumeric(c) && c!='-' && c!='_') return false;
    return true;
}
static bool containsWildcard(const String& s) { return s.indexOf('*') != -1; }

// ── Route handlers ─────────────────────────────────────────────────────────
static void handleRoot() {
    g_server.send(200, "text/html", buildPortalPage(*g_cfg));
}

static void handleCheckAuth() {
    if (g_server.hasArg("pwd") &&
        g_server.arg("pwd") == ADMIN_PASSWORD) {
        g_server.send(200, "text/plain", "OK");
    } else {
        g_server.send(403, "text/plain", "FAIL");
    }
}

static void handleGetMQTT() {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<384> doc;
#endif
    doc["server"] = g_cfg->mqttServer;
    doc["port"]   = g_cfg->mqttPort;
    doc["device"] = g_cfg->deviceID;
    doc["tenant"] = g_cfg->tenantID;
    doc["user"]   = g_cfg->mqttUser;
    char buf[384];
    serializeJson(doc, buf, sizeof(buf));
    g_server.send(200, "application/json", buf);
}

static void handleSaveAll() {
    DeviceConfig updated = *g_cfg;
    bool valid = true;
    String errMsg;

    if (g_server.hasArg("ssid") && !g_server.arg("ssid").isEmpty()) {
        updated.wifiSSID = g_server.arg("ssid");
    }
    if (g_server.hasArg("pass") && !g_server.arg("pass").isEmpty()) {
        updated.wifiPass = g_server.arg("pass");
    }

    if (g_server.hasArg("server")) {
        String s = g_server.arg("server");
        if (!s.isEmpty() && !containsWildcard(s)) updated.mqttServer = s;
    }
    if (g_server.hasArg("port")) {
        String p = g_server.arg("port");
        if (!p.isEmpty() && !containsWildcard(p)) {
            if (!validPort(p)) { valid = false; errMsg = "Invalid port"; }
            else updated.mqttPort = p.toInt();
        }
    }
    if (g_server.hasArg("device")) {
        String d = g_server.arg("device");
        if (!d.isEmpty() && !containsWildcard(d)) {
            if (!validDeviceID(d)) { valid = false; errMsg = "Invalid device ID"; }
            else updated.deviceID = d;
        }
    }
    if (g_server.hasArg("tenant")) {
        String t = g_server.arg("tenant");
        if (!t.isEmpty()) updated.tenantID = t;
    }
    if (g_server.hasArg("mqttuser")) {
        String u = g_server.arg("mqttuser");
        if (!u.isEmpty()) updated.mqttUser = u;
    }
    if (g_server.hasArg("mqttpass")) {
        String mp = g_server.arg("mqttpass");
        if (!mp.isEmpty()) updated.mqttPass = mp;
    }

    if (!valid) {
        g_server.send(400, "application/json",
                      "{\"error\":\"" + errMsg + "\"}");
        return;
    }

    *g_cfg = updated;
    Storage::save(updated);

    g_server.send(200, "text/plain", "OK");

    WebPortal::pendingRestart  = true;
    WebPortal::restartFlagTime = millis();
}

static void handleProvision() {
    if (!g_server.hasArg("pwd") || g_server.arg("pwd") != ADMIN_PASSWORD) {
        g_server.send(403, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    String body = g_server.arg("plain");
    if (body.isEmpty()) {
        g_server.send(400, "application/json", "{\"error\":\"Empty payload\"}");
        return;
    }
    if ((int)body.length() > PROVISION_PAYLOAD_MAX) {
        g_server.send(413, "application/json", "{\"error\":\"Payload too large\"}");
        return;
    }

    QrPayload payload;
    String error;
    if (!QrProvision::parsePayload(body, payload, error)) {
        g_server.send(400, "application/json",
                      "{\"error\":\"" + error + "\"}");
        return;
    }

    QrProvision::applyToConfig(payload, *g_cfg);
    Storage::save(*g_cfg);

    g_server.send(200, "application/json",
                  "{\"status\":\"provisioned\",\"restart\":true}");

    WebPortal::pendingRestart  = true;
    WebPortal::restartFlagTime = millis();
}

// ── Public API ─────────────────────────────────────────────────────────────
void WebPortal::init(DeviceConfig& cfg) {
    g_cfg = &cfg;
    g_server.on("/",          handleRoot);
    g_server.on("/checkauth", HTTP_POST, handleCheckAuth);
    g_server.on("/getmqtt",   HTTP_GET,  handleGetMQTT);
    g_server.on("/saveall",   HTTP_POST, handleSaveAll);
    g_server.on("/provision", HTTP_POST, handleProvision);
    g_server.begin();
    Serial.println("[WebPortal] Started on port 80");
}

void WebPortal::tick() {
    g_server.handleClient();
}
