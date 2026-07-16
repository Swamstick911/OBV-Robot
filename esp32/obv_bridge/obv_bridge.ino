/*
 * OBV Robot — ESP32 WiFi bridge
 * -----------------------------
 * Replaces the HC-05. The ESP32:
 *   - hosts the "Sonar HUD" control website + a WebSocket,
 *   - forwards command characters from the browser to the Arduino Uno,
 *   - relays the Uno's A<angle>D<distance> telemetry back to the browser (radar).
 *
 * WiFi: tries your home network (STA) if credentials are set, otherwise starts
 * its own hotspot "OBV-Robot" (AP). Either way, open the shown IP in a browser.
 *
 * Serial link to the Uno (9600 baud):
 *   ESP32 GPIO17 (TX2) --------------> Uno RX (pin 0)      commands out
 *   ESP32 GPIO16 (RX2) <--[1k]-------- Uno TX (pin 1)      telemetry in
 *   ESP32 GND -- Uno GND, ESP32 5V(VIN) -- Uno 5V
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ---- WiFi ----
// Set STA_SSID to join your home WiFi; leave "" to use the robot's own hotspot.
const char* STA_SSID = "";
const char* STA_PASS = "";
const char* AP_SSID  = "OBV-Robot";
const char* AP_PASS  = "obvrobot123";   // min 8 chars; connect to this hotspot

// ---- Serial to Uno ----
const uint32_t UNO_BAUD = 9600;
const int PIN_RX2 = 16;   // from Uno TX (via 1k resistor)
const int PIN_TX2 = 17;   // to Uno RX

WebServer server(80);
WebSocketsServer webSocket(81);
String telemetryBuffer;

const char INDEX_HTML[] PROGMEM = R"HTMLPAGE(
<!doctype html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>OBV Robot</title>
<style>
:root{--bg:#080B11;--panel:#121A24;--border:#1E2C3A;--cyan:#22E5C7;--amber:#FFB020;--red:#FF4D5E;--green:#3DDC84;--text:#E6F1F5;--dim:#6B8299;--mono:ui-monospace,Consolas,monospace}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;background:radial-gradient(700px 400px at 50% -10%,#0d1622,#080B11 60%),#080B11;color:var(--text);font-family:system-ui,sans-serif;min-height:100vh;padding:12px;display:flex;flex-direction:column;gap:12px;max-width:460px;margin:0 auto}
.head{display:flex;align-items:center}
.logo{font-family:var(--mono);font-size:19px;font-weight:700}.logo b{color:var(--cyan)}
.pill{margin-left:auto;display:inline-flex;align-items:center;gap:6px;font-family:var(--mono);font-size:11px;padding:7px 12px;border-radius:20px;background:var(--panel);border:1px solid}
.pill .dot{width:7px;height:7px;border-radius:50%;background:currentColor;box-shadow:0 0 8px currentColor}
.on{color:var(--green);border-color:rgba(61,220,132,.5)}.off{color:var(--red);border-color:rgba(255,77,94,.5)}
.radarbox{background:var(--panel);border:1px solid var(--border);border-radius:16px;padding:12px 12px 8px}
.rhead{display:flex;align-items:center;gap:6px;font-family:var(--mono);font-size:12px;color:var(--dim)}
.rhead .val{margin-left:auto;font-size:14px;color:var(--cyan)}.rhead .val.near{color:var(--red)}
canvas{width:100%;display:block;margin-top:8px}
.modes{display:flex;gap:4px;padding:4px;background:var(--panel);border:1px solid var(--border);border-radius:12px}
.mode{flex:1;border:1px solid transparent;border-radius:9px;background:none;color:var(--dim);padding:9px 4px;font-family:var(--mono);font-size:11px;cursor:pointer}
.mode.active{color:var(--cyan);border-color:var(--cyan);background:rgba(34,229,199,.14)}
.view{display:none;flex-direction:column;align-items:center;gap:14px;padding:6px 0}
.view.active{display:flex}
.dpad{display:grid;grid-template-columns:repeat(3,72px);grid-template-rows:repeat(3,72px);gap:10px}
.key{border-radius:16px;background:var(--panel);border:1.5px solid var(--border);display:flex;align-items:center;justify-content:center;color:var(--cyan);font-size:30px;user-select:none;cursor:pointer}
.key.stop{color:var(--red);font-family:var(--mono);font-weight:700;font-size:13px}
.key.blank{background:none;border:none}
.key.press{background:rgba(34,229,199,.18);border-color:var(--cyan);box-shadow:0 0 16px rgba(34,229,199,.45)}
.key.stop.press{background:rgba(255,77,94,.18);border-color:var(--red);box-shadow:0 0 16px rgba(255,77,94,.45)}
.speed{display:flex;align-items:center;gap:10px;width:100%;font-family:var(--mono);font-size:11px;color:var(--dim)}
.speed input{flex:1;accent-color:var(--cyan)}.speed .num{width:22px;text-align:right;color:var(--cyan);font-size:15px}
.mic{width:120px;height:120px;border-radius:50%;background:var(--panel);border:2px solid var(--border);display:flex;align-items:center;justify-content:center;font-size:50px;cursor:pointer}
.mic.live{background:rgba(34,229,199,.18);border-color:var(--cyan);box-shadow:0 0 30px 4px rgba(34,229,199,.4)}
.big{width:150px;height:150px;border-radius:50%;background:rgba(34,229,199,.14);border:2px solid var(--cyan);color:var(--cyan);display:flex;flex-direction:column;align-items:center;justify-content:center;font-family:var(--mono);font-size:15px;cursor:pointer;box-shadow:0 0 24px 2px rgba(34,229,199,.3)}
.big.stop{background:rgba(255,77,94,.14);border-color:var(--red);color:var(--red);box-shadow:0 0 24px 2px rgba(255,77,94,.3)}
.muted{font-family:var(--mono);font-size:11px;color:var(--dim);text-align:center}
.heard{font-size:16px;min-height:20px}
</style></head><body>
<div class="head"><div class="logo"><b>OBV</b> ROBOT</div><div id="pill" class="pill off"><span class="dot"></span><span id="pillt">OFFLINE</span></div></div>
<div class="radarbox"><div class="rhead">SONAR<span id="dist" class="val">--- cm</span></div><canvas id="radar" width="620" height="320"></canvas></div>
<div class="modes"><button class="mode active" data-m="manual">MANUAL</button><button class="mode" data-m="voice">VOICE</button><button class="mode" data-m="auto">AUTO</button></div>
<div id="manual" class="view active">
  <div class="dpad">
    <div class="key blank"></div><div class="key" data-k="F">&#9650;</div><div class="key blank"></div>
    <div class="key" data-k="L">&#9664;</div><div class="key stop" data-k="S">STOP</div><div class="key" data-k="R">&#9654;</div>
    <div class="key blank"></div><div class="key" data-k="B">&#9660;</div><div class="key blank"></div>
  </div>
  <div class="speed">SPEED<input id="speed" type="range" min="0" max="9" value="6"><span id="sv" class="num">6</span></div>
</div>
<div id="voice" class="view"><div id="mic" class="mic">&#127908;</div><div id="vstatus" class="muted">Tap to speak</div><div id="heard" class="heard"></div><div class="muted">Try: "forward" &middot; "left" &middot; "stop" &middot; "auto"</div></div>
<div id="auto" class="view"><div id="engage" class="big"><span id="engt">ENGAGE</span></div><div class="muted">Robot drives itself and avoids obstacles</div></div>
<script>
var ws,wsOK=false;
function send(c){if(ws&&ws.readyState===1)ws.send(c)}
function setPill(o){var p=document.getElementById('pill');p.className='pill '+(o?'on':'off');document.getElementById('pillt').textContent=o?'ONLINE':'OFFLINE'}
function connect(){
  ws=new WebSocket('ws://'+location.hostname+':81/');
  ws.onopen=function(){wsOK=true;setPill(true)};
  ws.onclose=function(){wsOK=false;setPill(false);setTimeout(connect,1500)};
  ws.onerror=function(){ws.close()};
  ws.onmessage=function(e){onTelemetry(e.data)};
}
connect();
// modes
var modes=document.querySelectorAll('.mode'),views={manual:document.getElementById('manual'),voice:document.getElementById('voice'),auto:document.getElementById('auto')};
modes.forEach(function(m){m.onclick=function(){send('S');modes.forEach(function(x){x.classList.remove('active')});m.classList.add('active');for(var k in views)views[k].classList.remove('active');views[m.dataset.m].classList.add('active');if(m.dataset.m==='auto')autoOn=false,setEngage()}});
// dpad
document.querySelectorAll('.key[data-k]').forEach(function(k){
  var c=k.dataset.k;
  function down(e){e.preventDefault();k.classList.add('press');send(c)}
  function up(){k.classList.remove('press');if(c!=='S')send('S')}
  k.addEventListener('touchstart',down,{passive:false});k.addEventListener('touchend',up);
  k.addEventListener('mousedown',down);k.addEventListener('mouseup',up);k.addEventListener('mouseleave',function(){if(k.classList.contains('press'))up()});
});
// speed
var sp=document.getElementById('speed');sp.oninput=function(){document.getElementById('sv').textContent=sp.value;send(sp.value)};
// auto
var autoOn=false;function setEngage(){var b=document.getElementById('engage');b.className='big'+(autoOn?' stop':'');document.getElementById('engt').textContent=autoOn?'STOP':'ENGAGE'}
document.getElementById('engage').onclick=function(){autoOn=!autoOn;setEngage();send(autoOn?'T':'S')};
// voice
var mic=document.getElementById('mic'),SR=window.SpeechRecognition||window.webkitSpeechRecognition,rec=null,listening=false;
function mapCmd(p){p=p.toLowerCase();if(/stop|halt|brake/.test(p))return'S';if(/auto|avoid|obstacle/.test(p))return'T';if(/forward|ahead|go|front/.test(p))return'F';if(/back|reverse/.test(p))return'B';if(/left/.test(p))return'L';if(/right/.test(p))return'R';return null}
mic.onclick=function(){
  if(!SR){document.getElementById('vstatus').textContent='Voice not supported in this browser (use Chrome)';return}
  if(listening){rec&&rec.stop();return}
  rec=new SR();rec.lang='en-US';rec.interimResults=true;
  rec.onstart=function(){listening=true;mic.classList.add('live');document.getElementById('vstatus').textContent='Listening...'};
  rec.onend=function(){listening=false;mic.classList.remove('live');document.getElementById('vstatus').textContent='Tap to speak'};
  rec.onresult=function(e){var t=e.results[e.results.length-1][0].transcript;document.getElementById('heard').textContent='"'+t+'"';if(e.results[e.results.length-1].isFinal){var c=mapCmd(t);if(c){send(c);document.getElementById('heard').textContent+=' -> '+c}}};
  rec.start();
};
// radar
var cv=document.getElementById('radar'),ctx=cv.getContext('2d'),W=cv.width,H=cv.height,cx=W/2,cy=H-6,R=Math.min(W/2,H)*0.97,MAXR=120,TH=30;
var blips={},lastAngle=90,lastDist=null,lastMsg=0;
function onTelemetry(s){var m=/^A(\d{1,3})D(\d{1,4})$/.exec(s.trim());if(!m)return;var a=+m[1],d=+m[2];blips[a]={d:d,t:performance.now()};lastAngle=a;lastDist=d;lastMsg=performance.now();
  var el=document.getElementById('dist');el.textContent=d+' cm';el.className='val'+(d<TH?' near':'')}
function proj(a,d){var r=Math.min(d,MAXR)/MAXR*R,rad=a*Math.PI/180;return[cx+r*Math.cos(rad),cy-r*Math.sin(rad)]}
function draw(t){ctx.clearRect(0,0,W,H);
  ctx.beginPath();ctx.moveTo(cx,cy);ctx.arc(cx,cy,R,Math.PI,2*Math.PI);ctx.closePath();ctx.fillStyle='#0E141D';ctx.fill();
  ctx.font='11px monospace';for(var i=1;i<=4;i++){var rr=R*i/4;ctx.beginPath();ctx.arc(cx,cy,rr,Math.PI,2*Math.PI);ctx.strokeStyle='rgba(15,90,82,.5)';ctx.stroke();ctx.fillStyle='#6B8299';ctx.fillText(MAXR*i/4,cx+4,cy-rr+13)}
  [0,45,90,135,180].forEach(function(a){var p=proj(a,MAXR);ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(p[0],p[1]);ctx.strokeStyle='rgba(15,90,82,.35)';ctx.stroke()});
  var live=performance.now()-lastMsg<2000;
  if(live){var p=proj(lastAngle,MAXR),g=ctx.createLinearGradient(cx,cy,p[0],p[1]);g.addColorStop(0,'rgba(34,229,199,0)');g.addColorStop(1,'rgba(34,229,199,.9)');ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(p[0],p[1]);ctx.strokeStyle=g;ctx.lineWidth=2.5;ctx.stroke();ctx.lineWidth=1}
  for(var a in blips){var b=blips[a],age=(t-b.t)/2500;if(age>1){delete blips[a];continue}if(b.d<=0||b.d>MAXR)continue;var q=proj(+a,b.d),col=b.d<TH?'#FF4D5E':(b.d<TH*2?'#FFB020':'#22E5C7');ctx.globalAlpha=(1-age)*.3;ctx.beginPath();ctx.arc(q[0],q[1],7,0,7);ctx.fillStyle=col;ctx.fill();ctx.globalAlpha=1-age;ctx.beginPath();ctx.arc(q[0],q[1],3.2,0,7);ctx.fillStyle=col;ctx.fill()}
  ctx.globalAlpha=1;ctx.beginPath();ctx.arc(cx,cy,4,0,7);ctx.fillStyle='#22E5C7';ctx.fill();
  if(!live){ctx.fillStyle='#6B8299';ctx.font='12px monospace';ctx.textAlign='center';ctx.fillText('awaiting telemetry',cx,cy-R/2);ctx.textAlign='left'}
  requestAnimationFrame(draw)}
requestAnimationFrame(draw);
</script></body></html>
)HTMLPAGE";

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT && length > 0) {
    Serial2.write(payload, length);   // forward command chars to the Uno
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
    Serial.print("STA connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP mode. Connect to '");
    Serial.print(AP_SSID);
    Serial.print("' then open http://");
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

  // Relay Uno telemetry lines to all connected browsers.
  while (Serial2.available()) {
    char c = (char)Serial2.read();
    if (c == '\n') {
      telemetryBuffer.trim();
      if (telemetryBuffer.length() > 0) webSocket.broadcastTXT(telemetryBuffer);
      telemetryBuffer = "";
    } else if (c != '\r') {
      telemetryBuffer += c;
      if (telemetryBuffer.length() > 64) telemetryBuffer = "";
    }
  }
}
