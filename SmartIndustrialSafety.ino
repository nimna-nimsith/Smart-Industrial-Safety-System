/*
 * ============================================================
 *   SMART INDUSTRIAL SAFETY & MONITORING SYSTEM
 *   Microcontroller : ESP32-C3 Mini
 *   Version         : 3.3 — Data Fix (JSON + CORS)
 * ============================================================
 *
 *  PIN CONFIGURATION
 *  ┌─────────────────┬───────────────────┐
 *  │ OLED SDA        │ GPIO 8            │
 *  │ OLED SCL        │ GPIO 9            │
 *  │ DHT11           │ GPIO 2            │
 *  │ PIR Sensor      │ GPIO 3            │
 *  │ Gas Sensor (MQ) │ GPIO 0 (Analog)   │
 *  │ Relay           │ GPIO 4 (ActiveLOW)│
 *  │ Buzzer          │ GPIO 5            │
 *  └─────────────────┴───────────────────┘
 *
 *  LIBRARIES needed:
 *   - U8g2               (by oliver)
 *   - DHT sensor library (by Adafruit)
 *   - Adafruit Unified Sensor
 * ============================================================
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

// ─────────────────────────────────────────────
//  WiFi CREDENTIALS
// ─────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_HOTSPOT_NAME";
const char* WIFI_PASSWORD = "YOUR_HOTSPOT_PASSWORD";

// ─────────────────────────────────────────────
//  OLED — SH1106 128x64 Software I2C
// ─────────────────────────────────────────────
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(
  U8G2_R0, /* SCL= */ 9, /* SDA= */ 8, /* RST= */ U8X8_PIN_NONE
);

// ─────────────────────────────────────────────
//  PINS
// ─────────────────────────────────────────────
#define DHT_PIN      2
#define PIR_PIN      3
#define GAS_PIN      0
#define RELAY_PIN    4
#define BUZZER_PIN   5
#define RELAY_ON     LOW
#define RELAY_OFF    HIGH

// ─────────────────────────────────────────────
//  THRESHOLDS
// ─────────────────────────────────────────────
#define GAS_THRESHOLD   1500
#define TEMP_THRESHOLD  40.0
#define BUZZER_FREQ     1000
#define BUZZER_ON_MS    250
#define BUZZER_OFF_MS   200

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

WebServer server(80);

// ─────────────────────────────────────────────
//  STATE
// ─────────────────────────────────────────────
float    temperature    = 0.0;
float    humidity       = 0.0;
int      gasLevel       = 0;
bool     motionDetected = false;
bool     dangerState    = false;
bool     dangerGas      = false;
bool     dangerTemp     = false;
bool     dangerMotion   = false;
String   ipAddress      = "";
unsigned long systemUptime = 0;
int      alertCount     = 0;
bool     wifiConnected  = false;

unsigned long lastSensorRead    = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastBuzzerToggle  = 0;
unsigned long lastUptimeTick    = 0;
bool          buzzerOn          = false;

const unsigned long SENSOR_INTERVAL  = 1000;
const unsigned long DISPLAY_INTERVAL = 400;

// ─────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ─────────────────────────────────────────────
void readSensors();
void evaluateSafety();
void handleBuzzer(unsigned long now);
void updateDisplay();
void drawSafeScreen();
void drawDangerScreen();
void showBootScreen();
void showWiFiConnecting(int attempt, int total);
void showWiFiConnected();
void showWiFiFailed();
void logToSerial();
void handleRoot();
void handleData();
void handleNotFound();
bool connectWiFi();

// ─────────────────────────────────────────────
//  HTML — embedded in flash
// ─────────────────────────────────────────────
const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Industrial Safety Monitor</title>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Share+Tech+Mono&display=swap" rel="stylesheet">
<style>
:root{
  --safe:#00ff88;--danger:#ff3c3c;--warn:#ffaa00;
  --bg:#050a0f;--panel:#0a1520;--border:#0d2d40;
  --text:#b0cfe0;--accent:#00c8ff;--grid:rgba(0,200,255,0.04);
}
*{margin:0;padding:0;box-sizing:border-box}
body{background:var(--bg);color:var(--text);font-family:'Share Tech Mono',monospace;min-height:100vh;overflow-x:hidden}
body::before{content:'';position:fixed;inset:0;background-image:linear-gradient(var(--grid) 1px,transparent 1px),linear-gradient(90deg,var(--grid) 1px,transparent 1px);background-size:40px 40px;pointer-events:none;z-index:0}
body::after{content:'';position:fixed;inset:0;background:repeating-linear-gradient(0deg,transparent,transparent 2px,rgba(0,0,0,0.08) 2px,rgba(0,0,0,0.08) 4px);pointer-events:none;z-index:0}
.wrapper{position:relative;z-index:1;max-width:1100px;margin:0 auto;padding:20px 16px 40px}
header{text-align:center;padding:24px 0 28px}
.hbadge{display:inline-block;border:1px solid var(--accent);color:var(--accent);font-size:.65rem;letter-spacing:4px;padding:4px 16px;text-transform:uppercase;margin-bottom:12px;animation:pb 2s infinite}
@keyframes pb{0%,100%{border-color:var(--accent)}50%{border-color:transparent}}
h1{font-family:'Orbitron',monospace;font-size:clamp(1.4rem,4vw,2.4rem);font-weight:900;color:#fff;letter-spacing:3px;text-shadow:0 0 30px rgba(0,200,255,.5)}
.sub{font-size:.75rem;letter-spacing:3px;color:var(--accent);margin-top:6px;opacity:.8}

/* STATUS */
.sbanner{border-radius:8px;padding:18px 24px;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:12px;margin-bottom:24px;border:2px solid;transition:all .4s;overflow:hidden}
.sbanner.safe{background:rgba(0,255,136,.06);border-color:var(--safe);box-shadow:0 0 30px rgba(0,255,136,.15)}
.sbanner.danger{background:rgba(255,60,60,.08);border-color:var(--danger);animation:dp .8s infinite alternate}
@keyframes dp{from{box-shadow:0 0 30px rgba(255,60,60,.2)}to{box-shadow:0 0 60px rgba(255,60,60,.5)}}
.slabel{font-family:'Orbitron',monospace;font-size:clamp(1.2rem,3vw,1.8rem);font-weight:900;letter-spacing:4px}
.sbanner.safe   .slabel{color:var(--safe)}
.sbanner.danger .slabel{color:var(--danger)}
.smeta{font-size:.75rem;opacity:.7;text-align:right}
.dot{width:12px;height:12px;border-radius:50%;display:inline-block;margin-right:10px;vertical-align:middle;animation:blink 1s infinite}
.safe   .dot{background:var(--safe);box-shadow:0 0 10px var(--safe)}
.danger .dot{background:var(--danger);box-shadow:0 0 10px var(--danger)}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.3}}

/* MACHINE */
.mrow{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-bottom:24px}
.mc{background:var(--panel);border:1px solid var(--border);border-radius:10px;padding:20px;text-align:center}
.mlabel{font-size:.65rem;letter-spacing:3px;text-transform:uppercase;color:var(--accent);opacity:.7;margin-bottom:10px}
.mstate{font-family:'Orbitron',monospace;font-size:1.3rem;font-weight:700;letter-spacing:2px}
.mstate.on {color:var(--safe);  text-shadow:0 0 20px var(--safe)}
.mstate.off{color:var(--danger);text-shadow:0 0 20px var(--danger)}

/* CARDS */
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:16px;margin-bottom:24px}
.card{background:var(--panel);border:1px solid var(--border);border-radius:10px;padding:20px;position:relative;overflow:hidden;transition:border-color .3s,box-shadow .3s}
.card::before{content:'';position:absolute;top:0;left:0;right:0;height:2px;background:linear-gradient(90deg,transparent,var(--accent),transparent);opacity:.5}
.card:hover{border-color:var(--accent);box-shadow:0 0 20px rgba(0,200,255,.1)}
.card.alert{border-color:var(--danger)!important;box-shadow:0 0 20px rgba(255,60,60,.2)!important}
.card.alert::before{background:linear-gradient(90deg,transparent,var(--danger),transparent);opacity:1}
.cicon{font-size:1.8rem;margin-bottom:8px;display:block}
.clabel{font-size:.65rem;letter-spacing:3px;text-transform:uppercase;color:var(--accent);opacity:.8;margin-bottom:6px}
.cval{font-family:'Orbitron',monospace;font-size:clamp(1.6rem,3vw,2.2rem);font-weight:700;color:#fff;line-height:1}
.cunit{font-size:.8rem;color:var(--text);margin-left:4px;opacity:.7}
.cstatus{font-size:.7rem;margin-top:8px;padding:3px 10px;border-radius:20px;display:inline-block;letter-spacing:2px}
.sok   {background:rgba(0,255,136,.12);color:var(--safe);  border:1px solid var(--safe)}
.salert{background:rgba(255,60,60,.12); color:var(--danger);border:1px solid var(--danger)}
.smot  {background:rgba(255,170,0,.12); color:var(--warn);  border:1px solid var(--warn)}
.gw{margin-top:10px}
.gt{background:rgba(255,255,255,.06);border-radius:4px;height:5px;overflow:hidden}
.gf{height:100%;border-radius:4px;transition:width .6s,background .4s}

/* INFO TILES */
.irow{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px;margin-bottom:24px}
.it{background:var(--panel);border:1px solid var(--border);border-radius:8px;padding:14px 18px;display:flex;align-items:center;gap:14px}
.iti{font-size:1.3rem;opacity:.8}
.itl{font-size:.6rem;letter-spacing:2px;opacity:.6;text-transform:uppercase}
.itv{font-size:.95rem;color:#fff;margin-top:2px}

/* LOG */
.stitle{font-family:'Orbitron',monospace;font-size:.75rem;letter-spacing:4px;color:var(--accent);text-transform:uppercase;margin-bottom:12px;padding-bottom:8px;border-bottom:1px solid var(--border)}
.alog{background:var(--panel);border:1px solid var(--border);border-radius:8px;padding:16px;min-height:80px;max-height:180px;overflow-y:auto;font-size:.78rem;line-height:1.8;margin-bottom:28px}
.alog::-webkit-scrollbar{width:4px}
.alog::-webkit-scrollbar-thumb{background:var(--border);border-radius:2px}
.ai{color:var(--danger)}
.ais{color:var(--safe)}
.na{color:var(--text);opacity:.4;font-style:italic}

/* CONNECTION STATUS BOX */
.connbox{background:var(--panel);border:1px solid var(--border);border-radius:8px;padding:12px 16px;margin-bottom:16px;font-size:.75rem;display:flex;align-items:center;gap:10px}
.connbox.ok  {border-color:var(--safe);color:var(--safe)}
.connbox.fail{border-color:var(--danger);color:var(--danger)}

footer{text-align:center;padding:24px 0 0;font-size:.65rem;letter-spacing:2px;opacity:.35;border-top:1px solid var(--border)}
.rbar{position:fixed;top:0;left:0;height:2px;background:var(--accent);box-shadow:0 0 8px var(--accent);animation:ra 2s linear infinite;z-index:999}
@keyframes ra{0%{width:0%;opacity:1}90%{width:100%;opacity:1}100%{width:100%;opacity:0}}
@media(max-width:480px){.mrow{grid-template-columns:1fr}.smeta{text-align:left}}
</style>
</head>
<body>
<div class="rbar"></div>
<div class="wrapper">

<header>
  <div class="hbadge">ESP32-C3 MINI &nbsp;|&nbsp; LIVE MONITOR</div>
  <h1>INDUSTRIAL SAFETY</h1>
  <div class="sub">REAL-TIME ENVIRONMENTAL MONITORING SYSTEM</div>
</header>

<!-- connection debug box -->
<div class="connbox" id="connBox">⏳ &nbsp;Connecting to ESP32 data feed...</div>

<div class="sbanner safe" id="sBanner">
  <div><span class="dot"></span><span class="slabel" id="sLabel">LOADING...</span></div>
  <div class="smeta" id="sMeta">Waiting for first data packet</div>
</div>

<div class="mrow">
  <div class="mc"><div class="mlabel">⚙ Machine / Motor</div><div class="mstate on" id="motor">---</div></div>
  <div class="mc"><div class="mlabel">🔔 Alarm System</div>  <div class="mstate on" id="alarm">---</div></div>
</div>

<div class="cards">
  <div class="card" id="cTemp">
    <span class="cicon">🌡</span>
    <div class="clabel">Temperature</div>
    <div><span class="cval" id="vTemp">--.-</span><span class="cunit">°C</span></div>
    <div class="gw"><div class="gt"><div class="gf" id="gTemp" style="width:0%;background:var(--safe)"></div></div></div>
    <div class="cstatus sok" id="sTemp">OK</div>
  </div>
  <div class="card" id="cHum">
    <span class="cicon">💧</span>
    <div class="clabel">Humidity</div>
    <div><span class="cval" id="vHum">--</span><span class="cunit">%</span></div>
    <div class="gw"><div class="gt"><div class="gf" id="gHum" style="width:0%;background:var(--accent)"></div></div></div>
    <div class="cstatus sok" id="sHum">NORMAL</div>
  </div>
  <div class="card" id="cGas">
    <span class="cicon">☁</span>
    <div class="clabel">Gas / Air Quality</div>
    <div><span class="cval" id="vGas">----</span><span class="cunit">ADC</span></div>
    <div class="gw"><div class="gt"><div class="gf" id="gGas" style="width:0%;background:var(--safe)"></div></div></div>
    <div class="cstatus sok" id="sGas">CLEAN</div>
  </div>
  <div class="card" id="cPir">
    <span class="cicon">👁</span>
    <div class="clabel">Motion / Presence</div>
    <div><span class="cval" id="vPir" style="font-size:1.2rem">---</span></div>
    <div class="cstatus sok" id="sPir">CLEAR</div>
  </div>
</div>

<div class="irow">
  <div class="it"><div class="iti">📡</div><div><div class="itl">IP Address</div>  <div class="itv" id="iIP">---</div></div></div>
  <div class="it"><div class="iti">⏱</div> <div><div class="itl">System Uptime</div><div class="itv" id="iUp">--:--:--</div></div></div>
  <div class="it"><div class="iti">⚠</div> <div><div class="itl">Alert Count</div>  <div class="itv" id="iAl">0</div></div></div>
  <div class="it"><div class="iti">🔄</div> <div><div class="itl">Last Updated</div> <div class="itv" id="iTime">--:--:--</div></div></div>
</div>

<div class="stitle">▶ EVENT LOG</div>
<div class="alog" id="alog"><div class="na">No events yet...</div></div>

<footer>
  <p>SMART INDUSTRIAL SAFETY MONITOR &nbsp;|&nbsp; ESP32-C3 MINI &nbsp;|&nbsp; v3.3</p>
  <p style="margin-top:4px">Auto-refresh 2s &nbsp;|&nbsp; Gas &gt;1500 = danger &nbsp;|&nbsp; Temp &gt;40°C = danger</p>
</footer>
</div>

<script>
const log=[];let lastOk=null;

function uptime(s){
  return String(Math.floor(s/3600)).padStart(2,'0')+':'+
         String(Math.floor(s%3600/60)).padStart(2,'0')+':'+
         String(s%60).padStart(2,'0');
}
function pushLog(msg,bad){
  const t=new Date().toLocaleTimeString();
  log.unshift({t,msg,bad});
  if(log.length>40)log.pop();
  document.getElementById('alog').innerHTML=log.map(e=>
    '<div class="ai'+(e.bad?'':' ais')+'">['+e.t+'] '+e.msg+'</div>'
  ).join('');
}

async function tick(){
  const cb=document.getElementById('connBox');
  try{
    // Use relative URL so it always hits the correct ESP32 IP
    const r=await fetch('/data',{cache:'no-store'});
    if(!r.ok){
      cb.className='connbox fail';
      cb.textContent='✘  Server replied with error: HTTP '+r.status;
      return;
    }
    const d=await r.json();

    cb.className='connbox ok';
    cb.textContent='✔  Live data — ESP32 connected  |  IP: '+d.ip;

    // Banner
    const bn=document.getElementById('sBanner');
    bn.className='sbanner '+(d.danger?'danger':'safe');
    document.getElementById('sLabel').textContent=d.danger?'⚠  DANGER  ⚠':'✔  ALL SYSTEMS SAFE';
    const causes=[];
    if(d.dangerGas)   causes.push('Gas Leak');
    if(d.dangerTemp)  causes.push('High Temp');
    if(d.dangerMotion)causes.push('Motion');
    document.getElementById('sMeta').textContent=d.danger
      ?'TRIGGERED BY: '+causes.join(', ')
      :'All readings within safe limits';

    if(lastOk!==null&&lastOk!==d.danger)
      pushLog(d.danger?'🔴 DANGER — '+causes.join(', '):'🟢 Returned to SAFE',d.danger);
    lastOk=d.danger;

    // Motor / Alarm
    const mo=document.getElementById('motor'),al=document.getElementById('alarm');
    mo.textContent=d.danger?'STOPPED':'RUNNING'; mo.className='mstate '+(d.danger?'off':'on');
    al.textContent=d.danger?'ACTIVE' :'STANDBY'; al.className='mstate '+(d.danger?'off':'on');

    // Temp
    const t=d.temperature;
    document.getElementById('vTemp').textContent=t.toFixed(1);
    const tc=d.dangerTemp?'var(--danger)':t>30?'var(--warn)':'var(--safe)';
    document.getElementById('gTemp').style.cssText='width:'+Math.min(t/80*100,100)+'%;background:'+tc;
    document.getElementById('cTemp').className='card'+(d.dangerTemp?' alert':'');
    document.getElementById('sTemp').className='cstatus '+(d.dangerTemp?'salert':'sok');
    document.getElementById('sTemp').textContent=d.dangerTemp?'HIGH!':'NORMAL';

    // Humidity
    document.getElementById('vHum').textContent=Math.round(d.humidity);
    document.getElementById('gHum').style.width=Math.min(d.humidity,100)+'%';
    document.getElementById('sHum').textContent=d.humidity>80?'HIGH':d.humidity<20?'LOW':'NORMAL';

    // Gas
    const g=d.gas;
    document.getElementById('vGas').textContent=g;
    const gc=d.dangerGas?'var(--danger)':g>1000?'var(--warn)':'var(--safe)';
    document.getElementById('gGas').style.cssText='width:'+Math.min(g/4095*100,100)+'%;background:'+gc;
    document.getElementById('cGas').className='card'+(d.dangerGas?' alert':'');
    document.getElementById('sGas').className='cstatus '+(d.dangerGas?'salert':'sok');
    document.getElementById('sGas').textContent=d.dangerGas?'LEAK!':g>1000?'ELEVATED':'CLEAN';

    // PIR
    document.getElementById('vPir').textContent=d.motion?'DETECTED':'CLEAR';
    document.getElementById('cPir').className='card'+(d.dangerMotion?' alert':'');
    document.getElementById('sPir').className='cstatus '+(d.motion?'smot':'sok');
    document.getElementById('sPir').textContent=d.motion?'MOTION!':'NO MOTION';

    // Info
    document.getElementById('iIP').textContent  =d.ip;
    document.getElementById('iUp').textContent  =uptime(d.uptime);
    document.getElementById('iAl').textContent  =d.alertCount;
    document.getElementById('iTime').textContent=new Date().toLocaleTimeString();

  }catch(e){
    cb.className='connbox fail';
    cb.textContent='✘  Cannot reach /data — '+e.message;
    document.getElementById('sLabel').textContent='NO CONNECTION';
    document.getElementById('sBanner').className='sbanner danger';
  }
}

tick();
setInterval(tick,2000);
</script>
</body>
</html>
)rawhtml";

// ─────────────────────────────────────────────
//  WEB HANDLERS
// ─────────────────────────────────────────────
void handleRoot() {
  server.send_P(200, "text/html", HTML_PAGE);
}

void handleData() {
  // Build JSON manually — no external library needed
  char json[320];
  snprintf(json, sizeof(json),
    "{"
    "\"temperature\":%.1f,"
    "\"humidity\":%.1f,"
    "\"gas\":%d,"
    "\"motion\":%s,"
    "\"danger\":%s,"
    "\"dangerGas\":%s,"
    "\"dangerTemp\":%s,"
    "\"dangerMotion\":%s,"
    "\"ip\":\"%s\","
    "\"uptime\":%lu,"
    "\"alertCount\":%d"
    "}",
    temperature,
    humidity,
    gasLevel,
    motionDetected ? "true" : "false",
    dangerState    ? "true" : "false",
    dangerGas      ? "true" : "false",
    dangerTemp     ? "true" : "false",
    dangerMotion   ? "true" : "false",
    ipAddress.c_str(),
    systemUptime,
    alertCount
  );

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.send(200, "application/json", json);

  // Debug print every data request
  Serial.print(F("[WEB] /data served → "));
  Serial.println(json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ─────────────────────────────────────────────
//  WiFi CONNECT
// ─────────────────────────────────────────────
bool connectWiFi() {
  Serial.println(F("\n========== WiFi DEBUG =========="));
  Serial.print(F("SSID     : ")); Serial.println(WIFI_SSID);
  Serial.print(F("Password : ")); Serial.println(WIFI_PASSWORD);

  WiFi.disconnect(true, true);
  delay(500);
  WiFi.mode(WIFI_STA);
  delay(200);
  WiFi.setHostname("ESP32-Safety");
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting"));
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
    showWiFiConnecting(attempts, 40);
    if (attempts % 10 == 0) {
      Serial.print(F(" [status=")); Serial.print(WiFi.status()); Serial.print(F("]"));
    }
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    ipAddress = WiFi.localIP().toString();
    Serial.print(F("✔ IP: ")); Serial.println(ipAddress);
    Serial.print(F("  RSSI: ")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
    return true;
  }
  Serial.print(F("✘ Failed. Status: ")); Serial.println(WiFi.status());
  return false;
}

// ─────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("\n=== Smart Industrial Safety  ==="));

  pinMode(PIR_PIN,    INPUT);
  pinMode(GAS_PIN,    INPUT);
  pinMode(RELAY_PIN,  OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN,  RELAY_ON);
  digitalWrite(BUZZER_PIN, LOW);

  u8g2.begin();
  showBootScreen();
  dht.begin();

  wifiConnected = connectWiFi();

  if (wifiConnected) {
    showWiFiConnected();
    server.on("/",        handleRoot);
    server.on("/data",    handleData);
    server.onNotFound(    handleNotFound);
    server.begin();
    Serial.println(F("Web server started"));
    Serial.print(F("Open: http://")); Serial.println(ipAddress);
  } else {
    showWiFiFailed();
  }
}

// ─────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  if (wifiConnected) server.handleClient();

  if (now - lastUptimeTick >= 1000) { lastUptimeTick = now; systemUptime++; }

  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    readSensors();
    evaluateSafety();
    logToSerial();
  }

  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = now;
    updateDisplay();
  }

  handleBuzzer(now);
}

// ─────────────────────────────────────────────
//  SENSORS
// ─────────────────────────────────────────────
void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity    = h;
  gasLevel       = analogRead(GAS_PIN);
  motionDetected = digitalRead(PIR_PIN);
}

void evaluateSafety() {
  dangerGas    = (gasLevel    >= GAS_THRESHOLD);
  dangerTemp   = (temperature >= TEMP_THRESHOLD);
  dangerMotion = motionDetected;
  bool prev    = dangerState;
  dangerState  = dangerGas || dangerTemp || dangerMotion;
  if (dangerState && !prev) alertCount++;
  if (dangerState) {
    digitalWrite(RELAY_PIN, RELAY_OFF);
  } else {
    digitalWrite(RELAY_PIN, RELAY_ON);
    noTone(BUZZER_PIN);
    buzzerOn = false;
  }
}

void handleBuzzer(unsigned long now) {
  if (!dangerState) return;
  unsigned long iv = buzzerOn ? BUZZER_ON_MS : BUZZER_OFF_MS;
  if (now - lastBuzzerToggle >= iv) {
    lastBuzzerToggle = now;
    buzzerOn = !buzzerOn;
    buzzerOn ? tone(BUZZER_PIN, BUZZER_FREQ) : noTone(BUZZER_PIN);
  }
}

// ─────────────────────────────────────────────
//  OLED
// ─────────────────────────────────────────────
void updateDisplay() {
  u8g2.clearBuffer();
  dangerState ? drawDangerScreen() : drawSafeScreen();
  u8g2.sendBuffer();
}

void drawSafeScreen() {
  u8g2.setDrawColor(1); u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0); u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(4,10,"INDUSTRIAL SAFETY SYS");
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(22,28,"** SAFE **");
  u8g2.drawHLine(0,31,128);
  u8g2.setFont(u8g2_font_6x10_tf);
  char l1[24],l2[24],l3[24];
  snprintf(l1,sizeof(l1),"Temp:%.1fC  Hum:%.0f%%",temperature,humidity);
  snprintf(l2,sizeof(l2),"Gas : %-4d",gasLevel);
  snprintf(l3,sizeof(l3),"Mot:%-3s  Motor:ON",motionDetected?"YES":"NO");
  u8g2.drawStr(0,42,l1); u8g2.drawStr(0,52,l2); u8g2.drawStr(0,62,l3);
  u8g2.setFont(u8g2_font_5x7_tf); u8g2.setDrawColor(0);
  u8g2.drawStr(82,10,wifiConnected?"WiFi:ON":"WiFi:--");
  u8g2.setDrawColor(1);
}

void drawDangerScreen() {
  static bool fl=false; fl=!fl;
  if(fl){u8g2.drawFrame(0,0,128,64);u8g2.drawFrame(2,2,124,60);}
  u8g2.setDrawColor(1); u8g2.drawBox(0,0,128,13);
  u8g2.setDrawColor(0); u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(4,10,"INDUSTRIAL SAFETY SYS");
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_ncenB12_tr); u8g2.drawStr(8,30,"!! DANGER !!");
  u8g2.setFont(u8g2_font_6x10_tf);
  int y=42;
  if(dangerGas)   {char b[22];snprintf(b,sizeof(b),"> Gas Leak! (%d)",gasLevel);    u8g2.drawStr(2,y,b);y+=10;}
  if(dangerTemp)  {char b[22];snprintf(b,sizeof(b),"> High Temp:%.1fC",temperature);u8g2.drawStr(2,y,b);y+=10;}
  if(dangerMotion){u8g2.drawStr(2,y,"> Motion Detected!");}
  u8g2.drawStr(2,63,"Motor:OFF  Alarm:ON");
}

void showBootScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(8,14,"SMART SAFETY");
  u8g2.setFont(u8g2_font_6x10_tf);    u8g2.drawStr(18,26,"MONITOR SYSTEM");
  u8g2.drawHLine(0,30,128);
  u8g2.drawStr(20,42,"ESP32-C3 Mini");
  u8g2.drawStr(28,52,"Version  3.3");
  u8g2.drawStr(16,63,"Initializing...");
  u8g2.sendBuffer(); delay(1500);
}

void showWiFiConnecting(int attempt, int total) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(14,16,"Connecting");
  u8g2.setFont(u8g2_font_6x10_tf);    u8g2.drawStr(30,28,"to WiFi...");
  char s[22]; snprintf(s,sizeof(s),"%.21s",WIFI_SSID);
  u8g2.drawStr(0,42,s);
  int bw=map(attempt,0,total,0,118);
  u8g2.drawFrame(4,50,120,8); u8g2.drawBox(5,51,bw,6);
  char c[16]; snprintf(c,sizeof(c),"Try %d / %d",attempt,total);
  u8g2.drawStr(28,63,c);
  u8g2.sendBuffer();
}

void showWiFiConnected() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(18,18,"WiFi  OK!");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0,32,"IP Address:");
  u8g2.drawStr(0,44,ipAddress.c_str());
  u8g2.drawStr(4,58,"Open in browser ^");
  u8g2.sendBuffer(); delay(3000);
}

void showWiFiFailed() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(10,16,"WiFi FAILED");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0,30,"Check Serial Monitor");
  u8g2.drawStr(0,42,"1=Bad SSID");
  u8g2.drawStr(0,52,"4=Bad Password");
  u8g2.drawStr(0,62,"6=Still Trying...");
  u8g2.sendBuffer(); delay(4000);
}

void logToSerial() {
  Serial.print(F("[DATA] T:")); Serial.print(temperature,1);
  Serial.print(F(" H:"));      Serial.print(humidity,1);
  Serial.print(F(" G:"));      Serial.print(gasLevel);
  Serial.print(F(" PIR:"));    Serial.print(motionDetected?"Y":"N");
  Serial.print(F(" Motor:"));  Serial.print(dangerState?"OFF":"ON ");
  Serial.print(F(" → "));      Serial.println(dangerState?"DANGER":"SAFE");
}
