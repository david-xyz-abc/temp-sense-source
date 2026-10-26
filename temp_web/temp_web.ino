// DS18B20 temperature dashboard for ESP32
// Wiring: red -> 3V3, black -> GND, yellow (data) -> D4, 4.7k resistor between D4 and 3V3
// Open http://temperature.local or the IP printed on serial

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Put your WiFi details here (2.4 GHz network only),
// or in a local secrets.h (gitignored) with the same two defines
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"
#endif

const int DATA_PIN = 4;

OneWire oneWire(DATA_PIN);
DallasTemperature sensors(&oneWire);
WebServer server(80);

float lastTemp = DEVICE_DISCONNECTED_C;
unsigned long lastRead = 0;

const char PAGE[] PROGMEM = R"html(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Temperature</title>
<style>
:root{--bg:#f6f7f9;--card:#fff;--fg:#1c1e21;--muted:#6b7280;--line:#ea580c;--grid:#e5e7eb}
@media(prefers-color-scheme:dark){:root{--bg:#111318;--card:#1b1e25;--fg:#e8eaed;--muted:#9aa0a6;--line:#fb923c;--grid:#2a2e37}}
body{margin:0;font-family:system-ui,sans-serif;background:var(--bg);color:var(--fg);display:flex;justify-content:center;padding:16px}
.card{background:var(--card);border-radius:14px;padding:20px;width:100%;max-width:520px;box-shadow:0 1px 3px rgba(0,0,0,.08)}
h1{font-size:18px;margin:0 0 12px}
.t{font-size:64px;font-weight:700;line-height:1;font-variant-numeric:tabular-nums}
.t small{font-size:28px;font-weight:600;color:var(--muted)}
.sub{color:var(--muted);font-size:14px;margin-top:6px;font-variant-numeric:tabular-nums}
.stats{display:flex;gap:16px;margin-top:16px}
.stat{flex:1;background:var(--bg);border-radius:10px;padding:10px 12px}
.stat b{display:block;font-size:20px;font-variant-numeric:tabular-nums}
.stat span{color:var(--muted);font-size:12px}
canvas{width:100%;height:160px;margin-top:18px}
.off{opacity:.4}
</style></head><body><div class="card" id="card">
<h1>Temperature</h1>
<div class="t" id="t">--<small>&deg;C</small></div>
<div class="sub" id="f">-- &deg;F</div>
<div class="stats">
 <div class="stat"><span>Min</span><b id="min">--</b></div>
 <div class="stat"><span>Max</span><b id="max">--</b></div>
 <div class="stat"><span>Status</span><b id="st" style="font-size:15px">waiting</b></div>
</div>
<canvas id="c"></canvas>
</div><script>
const hist=[],MAX=300,c=document.getElementById('c'),x=c.getContext('2d');let mn=1e9,mx=-1e9;
const $=id=>document.getElementById(id);
function draw(){
 const w=c.width=c.clientWidth*devicePixelRatio,h=c.height=c.clientHeight*devicePixelRatio,s=getComputedStyle(document.body);
 if(hist.length<2)return;
 let lo=Math.min(...hist),hi=Math.max(...hist);if(hi-lo<1){const m=(hi+lo)/2;lo=m-.5;hi=m+.5}
 const pad=(hi-lo)*.1;lo-=pad;hi+=pad;
 x.fillStyle=s.getPropertyValue('--muted');x.font=(11*devicePixelRatio)+'px system-ui';
 x.strokeStyle=s.getPropertyValue('--grid');x.lineWidth=1;
 [lo,(lo+hi)/2,hi].forEach(v=>{const y=h-(v-lo)/(hi-lo)*h;x.beginPath();x.moveTo(0,y);x.lineTo(w,y);x.stroke();
  x.fillText(v.toFixed(1)+'°',4,Math.min(h-4,Math.max(12*devicePixelRatio,y-4)))});
 x.strokeStyle=s.getPropertyValue('--line');x.lineWidth=2*devicePixelRatio;x.beginPath();
 hist.forEach((v,i)=>{const X=i/(MAX-1)*w,Y=h-(v-lo)/(hi-lo)*h;i?x.lineTo(X,Y):x.moveTo(X,Y)});x.stroke()}
async function tick(){
 try{const d=await (await fetch('/data')).json();
  $('card').classList.remove('off');
  if(d.ok){
   $('t').innerHTML=d.c.toFixed(2)+'<small>&deg;C</small>';$('f').textContent=(d.c*9/5+32).toFixed(1)+' °F';
   mn=Math.min(mn,d.c);mx=Math.max(mx,d.c);$('min').textContent=mn.toFixed(1)+'°';$('max').textContent=mx.toFixed(1)+'°';
   $('st').textContent='live';hist.push(d.c);if(hist.length>MAX)hist.shift();draw();
  }else $('st').textContent='sensor error';
 }catch(e){$('card').classList.add('off');$('st').textContent='disconnected'}}
setInterval(tick,1000);tick();addEventListener('resize',draw);
</script></body></html>)html";

void handleData() {
  bool ok = lastTemp != DEVICE_DISCONNECTED_C;
  server.send(200, "application/json",
              String("{\"ok\":") + (ok ? "true" : "false") + ",\"c\":" + String(lastTemp, 2) + "}");
}

void setup() {
  Serial.begin(115200);
  pinMode(DATA_PIN, INPUT_PULLUP);
  sensors.begin();
  Serial.printf("\nDS18B20 sensors found: %d\n", sensors.getDeviceCount());

  WiFi.mode(WIFI_STA);
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    Serial.printf("\nWiFi disconnected, reason %d (%s)\n",
                  info.wifi_sta_disconnected.reason,
                  WiFi.disconnectReasonName((wifi_err_reason_t)info.wifi_sta_disconnected.reason));
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  unsigned long attemptStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    // some routers/hotspots reject the first attempt; start over if stuck
    if (millis() - attemptStart > 15000) {
      Serial.println("\nRetrying WiFi");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      attemptStart = millis();
    }
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:   Serial.println("network not found"); break;
      case WL_CONNECT_FAILED:  Serial.println("connection failed"); break;
      default:                 Serial.print("."); break;
    }
  }
  Serial.printf("\nConnected! Open http://%s or http://temperature.local\n",
                WiFi.localIP().toString().c_str());

  MDNS.begin("temperature");
  MDNS.addService("http", "tcp", 80);
  server.on("/", [] { server.send_P(200, "text/html", PAGE); });
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
  if (millis() - lastRead > 1000) {
    lastRead = millis();
    sensors.requestTemperatures();
    lastTemp = sensors.getTempCByIndex(0);
  }
}
