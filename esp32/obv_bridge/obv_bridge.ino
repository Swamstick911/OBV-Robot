/*
 * OBV Robot — ESP32 WiFi bridge
 * -----------------------------
 * Hosts the control website + a WebSocket, and forwards command characters to
 * the Arduino over serial (ESP32 GPIO17 -> Arduino D10, one-way).
 *
 * WiFi: joins a home network if STA_SSID is set, else starts the "OBV-Robot"
 * hotspot. Open the shown IP in a browser.
 *
 * Control is TAP-TO-TOGGLE: tap a direction to latch it, tap again / tap STOP
 * to stop. (Sonar/radar dropped — control-focused build.)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* STA_SSID = "";
const char* STA_PASS = "";
const char* AP_SSID  = "OBV-Robot";
const char* AP_PASS  = "obvrobot123";

const uint32_t UNO_BAUD = 9600;
const int PIN_RX2 = 16;   // (unused now — no telemetry)
const int PIN_TX2 = 17;   // -> Arduino D10

WebServer server(80);
WebSocketsServer webSocket(81);

const char INDEX_HTML[] PROGMEM = R"HTMLPAGE(
<!doctype html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>OBV Robot</title>
<style>
:root{--bg:#080B11;--panel:#121A24;--panel2:#0f1620;--border:#1E2C3A;--cyan:#22E5C7;--amber:#FFB020;--red:#FF4D5E;--green:#3DDC84;--text:#E6F1F5;--dim:#6B8299;--mono:ui-monospace,Consolas,monospace}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;-webkit-user-select:none;user-select:none}
body{margin:0;background:radial-gradient(700px 420px at 50% -12%,#0e1826,#080B11 62%),#080B11;color:var(--text);font-family:system-ui,sans-serif;min-height:100vh;padding:16px 14px 24px;display:flex;flex-direction:column;gap:16px;max-width:440px;margin:0 auto}
.head{display:flex;align-items:center}
.logo{font-family:var(--mono);font-size:20px;font-weight:700;letter-spacing:1px}.logo b{color:var(--cyan)}
.pill{margin-left:auto;display:inline-flex;align-items:center;gap:6px;font-family:var(--mono);font-size:11px;letter-spacing:.5px;padding:7px 12px;border-radius:20px;background:var(--panel);border:1px solid}
.pill .dot{width:7px;height:7px;border-radius:50%;background:currentColor;box-shadow:0 0 8px currentColor}
.on{color:var(--green);border-color:rgba(61,220,132,.5)}.off{color:var(--red);border-color:rgba(255,77,94,.5)}
/* status */
.status{background:var(--panel);border:1px solid var(--border);border-radius:16px;padding:18px;display:flex;align-items:center;gap:16px}
.stArrow{width:56px;height:56px;border-radius:14px;display:flex;align-items:center;justify-content:center;font-size:30px;background:var(--panel2);border:1px solid var(--border);color:var(--dim);transition:all .15s}
.stArrow.go{color:var(--cyan);border-color:var(--cyan);box-shadow:0 0 18px rgba(34,229,199,.4)}
.stText .l{font-family:var(--mono);font-size:11px;color:var(--dim);letter-spacing:1px}
.stText .v{font-family:var(--mono);font-size:20px;font-weight:700;color:var(--text);margin-top:2px}
.stText .v.go{color:var(--cyan)}
/* mode tabs */
.modes{display:flex;gap:4px;padding:4px;background:var(--panel);border:1px solid var(--border);border-radius:12px}
.mode{flex:1;border:1px solid transparent;border-radius:9px;background:none;color:var(--dim);padding:10px 4px;font-family:var(--mono);font-size:12px;letter-spacing:.5px;cursor:pointer}
.mode.active{color:var(--cyan);border-color:var(--cyan);background:rgba(34,229,199,.14)}
.view{display:none;flex-direction:column;align-items:center;gap:16px}
.view.active{display:flex}
/* dpad */
.dpad{display:grid;grid-template-columns:repeat(3,86px);grid-template-rows:repeat(3,86px);gap:12px}
.key{border-radius:18px;background:var(--panel);border:1.5px solid var(--border);display:flex;align-items:center;justify-content:center;color:var(--cyan);font-size:36px;cursor:pointer;transition:all .12s}
.key:active{transform:scale(.95)}
.key.blank{background:none;border:none;cursor:default}
.key.on{background:rgba(34,229,199,.2);border-color:var(--cyan);box-shadow:0 0 20px rgba(34,229,199,.5)}
.key.stop{color:var(--red);font-family:var(--mono);font-weight:700;font-size:15px}
.key.stop:active{background:rgba(255,77,94,.2);border-color:var(--red)}
.hint{font-family:var(--mono);font-size:11px;color:var(--dim);text-align:center}
/* speed */
.speed{display:flex;align-items:center;gap:10px;width:100%;font-family:var(--mono);font-size:11px;color:var(--dim)}
.speed input{flex:1;accent-color:var(--cyan);height:26px}.speed .num{width:24px;text-align:right;color:var(--cyan);font-size:16px}
/* voice */
.mic{width:130px;height:130px;border-radius:50%;background:var(--panel);border:2px solid var(--border);display:flex;align-items:center;justify-content:center;font-size:56px;cursor:pointer;transition:all .15s}
.mic.live{background:rgba(34,229,199,.18);border-color:var(--cyan);box-shadow:0 0 34px 5px rgba(34,229,199,.4)}
.vtext{font-size:17px;min-height:22px}.vstatus{font-family:var(--mono);font-size:12px;color:var(--dim)}
</style></head><body>
<div class="head"><div class="logo"><b>OBV</b> ROBOT</div><div id="pill" class="pill off"><span class="dot"></span><span id="pillt">OFFLINE</span></div></div>

<div class="status"><div id="stArrow" class="stArrow">&#9632;</div><div class="stText"><div class="l">STATUS</div><div id="stVal" class="v">STOPPED</div></div></div>

<div class="modes"><button class="mode active" data-m="manual">MANUAL</button><button class="mode" data-m="voice">VOICE</button></div>

<div id="manual" class="view active">
  <div class="dpad">
    <div class="key blank"></div><div class="key" data-k="F">&#9650;</div><div class="key blank"></div>
    <div class="key" data-k="L">&#9664;</div><div class="key stop" data-k="S">STOP</div><div class="key" data-k="R">&#9654;</div>
    <div class="key blank"></div><div class="key" data-k="B">&#9660;</div><div class="key blank"></div>
  </div>
  <div class="speed">SPEED<input id="speed" type="range" min="0" max="9" value="6"><span id="sv" class="num">6</span></div>
  <div class="hint">Tap a direction to go &middot; tap it again or STOP to halt</div>
</div>

<div id="voice" class="view">
  <div id="mic" class="mic">&#127908;</div>
  <div id="vstatus" class="vstatus">Tap to speak</div>
  <div id="vtext" class="vtext"></div>
  <div class="hint">Say: "forward" &middot; "back" &middot; "left" &middot; "right" &middot; "stop"</div>
</div>

<script>
var ws,activeDir=null;
function send(c){if(ws&&ws.readyState===1)ws.send(c)}
function setPill(o){var p=document.getElementById('pill');p.className='pill '+(o?'on':'off');document.getElementById('pillt').textContent=o?'ONLINE':'OFFLINE'}
var LABEL={F:'FORWARD',B:'BACKWARD',L:'LEFT',R:'RIGHT'},ARROW={F:'▲',B:'▼',L:'◀',R:'▶'};
function render(){
  document.querySelectorAll('.key[data-k]').forEach(function(k){k.classList.toggle('on',k.dataset.k===activeDir&&activeDir!=='S')});
  var a=document.getElementById('stArrow'),v=document.getElementById('stVal');
  if(activeDir&&activeDir!=='S'){a.textContent=ARROW[activeDir];a.classList.add('go');v.textContent=LABEL[activeDir];v.classList.add('go')}
  else{a.textContent='■';a.classList.remove('go');v.textContent='STOPPED';v.classList.remove('go')}
}
function drive(c){ // tap-to-toggle
  if(c==='S'){activeDir=null;send('S')}
  else if(activeDir===c){activeDir=null;send('S')}   // tap same -> stop
  else{activeDir=c;send(c)}                            // new direction latches
  render();
}
function connect(){
  ws=new WebSocket('ws://'+location.hostname+':81/');
  ws.onopen=function(){setPill(true)};
  ws.onclose=function(){setPill(false);setTimeout(connect,1500)};
  ws.onerror=function(){ws.close()};
}
connect();
document.querySelectorAll('.key[data-k]').forEach(function(k){k.addEventListener('click',function(){drive(k.dataset.k)})});
// modes
var modes=document.querySelectorAll('.mode'),views={manual:document.getElementById('manual'),voice:document.getElementById('voice')};
modes.forEach(function(m){m.onclick=function(){drive('S');modes.forEach(function(x){x.classList.remove('active')});m.classList.add('active');for(var k in views)views[k].classList.remove('active');views[m.dataset.m].classList.add('active')}});
// speed
var sp=document.getElementById('speed');sp.oninput=function(){document.getElementById('sv').textContent=sp.value;send(sp.value)};
// voice
var mic=document.getElementById('mic'),SR=window.SpeechRecognition||window.webkitSpeechRecognition,rec=null,listening=false;
function mapCmd(p){p=p.toLowerCase();if(/stop|halt/.test(p))return'S';if(/forward|ahead|go|front/.test(p))return'F';if(/back|reverse/.test(p))return'B';if(/left/.test(p))return'L';if(/right/.test(p))return'R';return null}
mic.onclick=function(){
  if(!SR){document.getElementById('vstatus').textContent='Voice needs Chrome';return}
  if(listening){rec&&rec.stop();return}
  rec=new SR();rec.lang='en-US';rec.interimResults=true;
  rec.onstart=function(){listening=true;mic.classList.add('live');document.getElementById('vstatus').textContent='Listening...'};
  rec.onend=function(){listening=false;mic.classList.remove('live');document.getElementById('vstatus').textContent='Tap to speak'};
  rec.onresult=function(e){var r=e.results[e.results.length-1],t=r[0].transcript;document.getElementById('vtext').textContent='"'+t+'"';if(r.isFinal){var c=mapCmd(t);if(c)drive(c)}};
  rec.start();
};
render();
</script></body></html>
)HTMLPAGE";

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT && length > 0) {
    Serial2.write(payload, length);   // forward command chars to the Arduino
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(UNO_BAUD, SERIAL_8N1, PIN_RX2, PIN_TX2);

  bool staOK = false;
  if (strlen(STA_SSID) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(STA_SSID, STA_PASS);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(200);
    staOK = (WiFi.status() == WL_CONNECTED);
  }
  if (staOK) {
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP '");
    Serial.print(AP_SSID);
    Serial.print("' -> http://");
    Serial.println(WiFi.softAPIP());
  }

  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWsEvent);
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
