/*
 * FriendlyFire — Hub Feather ESP32 (Final Demo)
 *
 * Access Point + HTTP Server + WebSocket Server
 * Reads blaster ATmega UART on pin 7
 * Accepts WebSocket from vest FeatherS2 and browser
 * Broadcasts all game events to all connected clients
 *
 * Board: Adafruit Feather ESP32 V2
 * Libraries: WebSockets by Markus Sattler
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ===== Servers =====
WebServer server(80);
WebSocketsServer webSocket(81);

// ===== Blaster UART =====
const int blasterRxPin = 7;
const int blasterTxPin = -1;
String blasterLine;

// ===== Web page =====
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>FriendlyFire — Referee Console</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@400;600;700&display=swap');

  :root {
    --bg: #0a0c10;
    --surface: #12151c;
    --surface2: #1a1e28;
    --border: #2a2f3a;
    --text: #e0e0e0;
    --text-dim: #6b7280;
    --accent: #f59e0b;
    --red: #ef4444;
    --red-glow: rgba(239,68,68,0.3);
    --green: #22c55e;
    --green-glow: rgba(34,197,94,0.3);
    --blue: #3b82f6;
    --blue-glow: rgba(59,130,246,0.3);
    --purple: #a855f7;
  }

  * { margin:0; padding:0; box-sizing:border-box; }

  body {
    background: var(--bg); color: var(--text);
    font-family: 'Rajdhani', sans-serif; min-height: 100vh;
  }

  body::after {
    content:''; position:fixed; top:0;left:0;right:0;bottom:0;
    background: repeating-linear-gradient(transparent,transparent 2px,rgba(0,0,0,0.08) 2px,rgba(0,0,0,0.08) 4px);
    pointer-events:none; z-index:999;
  }

  .console { max-width:850px; margin:0 auto; padding:20px; }

  .header {
    display:flex; justify-content:space-between; align-items:center;
    padding:16px 20px; background:var(--surface);
    border:1px solid var(--border); border-radius:8px; margin-bottom:20px;
  }

  .header h1 {
    font-family:'Share Tech Mono',monospace;
    font-size:14px; letter-spacing:3px; text-transform:uppercase; color:var(--accent);
  }

  .connection-status {
    display:flex; align-items:center; gap:8px;
    font-family:'Share Tech Mono',monospace; font-size:12px; letter-spacing:1px;
  }

  .status-dot {
    width:8px; height:8px; border-radius:50%; background:var(--red);
    transition: background 0.3s, box-shadow 0.3s;
  }

  .status-dot.connected {
    background:var(--green); box-shadow:0 0 8px var(--green-glow);
    animation: pulse-dot 2s ease-in-out infinite;
  }

  @keyframes pulse-dot { 0%,100%{opacity:1} 50%{opacity:0.4} }

  .timer-section { text-align:center; padding:24px 0; margin-bottom:20px; }

  .match-timer {
    font-family:'Share Tech Mono',monospace;
    font-size:72px; font-weight:700; color:var(--text);
    text-shadow:0 0 30px rgba(255,255,255,0.1); line-height:1;
  }

  .match-status {
    font-family:'Share Tech Mono',monospace;
    font-size:13px; letter-spacing:4px; color:var(--text-dim);
    margin-top:8px; text-transform:uppercase;
  }

  .match-status.active { color:var(--green); }
  .match-status.ended { color:var(--red); }

  .players { display:grid; grid-template-columns:1fr 1fr; gap:16px; margin-bottom:20px; }

  .player-card {
    background:var(--surface); border:1px solid var(--border);
    border-radius:8px; padding:20px; overflow:hidden;
    transition: border-color 0.3s, box-shadow 0.3s;
  }

  .player-card.flash-shot { border-color:var(--accent); box-shadow:0 0 20px rgba(245,158,11,0.4); }
  .player-card.flash-reload { border-color:var(--green); box-shadow:0 0 20px var(--green-glow); }
  .player-card.flash-empty { border-color:var(--red); box-shadow:0 0 20px var(--red-glow); }
  .player-card.flash-hit { border-color:var(--red); box-shadow:0 0 20px var(--red-glow); }
  .player-card.flash-dead { border-color:var(--red); box-shadow:0 0 30px var(--red-glow); }
  .player-card.eliminated { opacity:0.5; }

  .player-name {
    font-family:'Share Tech Mono',monospace;
    font-size:13px; letter-spacing:3px; color:var(--text-dim);
    text-transform:uppercase; margin-bottom:16px;
  }

  .player-name .tag { color:var(--accent); }
  .player-name .tag.vest { color:var(--blue); }

  .stats-row { display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-bottom:14px; }
  .stats-row.tri { grid-template-columns:1fr 1fr 1fr; }

  .stat-box {
    background:var(--surface2); border-radius:6px; padding:12px; text-align:center;
  }

  .stat-label {
    font-size:9px; letter-spacing:2px; color:var(--text-dim);
    text-transform:uppercase; margin-bottom:4px;
    font-family:'Share Tech Mono',monospace;
  }

  .stat-value {
    font-family:'Share Tech Mono',monospace;
    font-size:32px; font-weight:700; line-height:1;
  }

  .stat-value.ammo { color:var(--blue); }
  .stat-value.ammo.low { color:var(--accent); }
  .stat-value.ammo.empty { color:var(--red); }
  .stat-value.shots { color:var(--accent); }
  .stat-value.reloads { color:var(--green); }
  .stat-value.lives { color:var(--green); }
  .stat-value.lives.low { color:var(--accent); }
  .stat-value.lives.critical { color:var(--red); }
  .stat-value.hits-taken { color:var(--red); }

  .bar-container {
    width:100%; height:6px; background:var(--surface2);
    border-radius:3px; overflow:hidden; margin-bottom:8px;
  }

  .bar-fill {
    height:100%; border-radius:3px;
    transition: width 0.4s ease, background 0.3s;
  }

  .bar-fill.ammo-bar { background:var(--blue); }
  .bar-fill.ammo-bar.low { background:var(--accent); }
  .bar-fill.ammo-bar.empty { background:var(--red); }

  .bar-fill.health-bar { background:var(--green); }
  .bar-fill.health-bar.low { background:var(--accent); }
  .bar-fill.health-bar.critical { background:var(--red); }

  .player-status {
    font-family:'Share Tech Mono',monospace;
    font-size:11px; letter-spacing:2px; text-align:center;
    padding:5px 10px; border-radius:4px; background:var(--surface2); color:var(--green);
  }

  .player-status.shooting { color:var(--accent); background:rgba(245,158,11,0.1); }
  .player-status.reloading { color:var(--green); background:rgba(34,197,94,0.1); }
  .player-status.empty-mag { color:var(--red); background:rgba(239,68,68,0.1); }
  .player-status.hit-status { color:var(--red); background:rgba(239,68,68,0.15); }
  .player-status.dead-status { color:var(--red); background:rgba(239,68,68,0.2); font-weight:700; }

  .event-log {
    background:var(--surface); border:1px solid var(--border);
    border-radius:8px; padding:16px 20px; margin-bottom:20px;
    max-height:240px; overflow-y:auto;
  }

  .event-log h2 {
    font-family:'Share Tech Mono',monospace;
    font-size:12px; letter-spacing:2px; color:var(--text-dim); margin-bottom:12px;
  }

  .event {
    display:flex; align-items:center; gap:10px; padding:5px 0;
    border-bottom:1px solid rgba(255,255,255,0.03);
    font-family:'Share Tech Mono',monospace; font-size:12px;
    animation: event-in 0.3s ease-out;
  }

  @keyframes event-in { from{opacity:0;transform:translateY(-8px)} to{opacity:1;transform:translateY(0)} }

  .event-time { color:var(--text-dim); font-size:11px; min-width:45px; }

  .event-tag {
    font-size:9px; padding:2px 5px; border-radius:3px;
    letter-spacing:1px; font-weight:700;
  }

  .event-tag.shot { background:rgba(245,158,11,0.2); color:var(--accent); }
  .event-tag.reload { background:rgba(34,197,94,0.2); color:var(--green); }
  .event-tag.empty { background:rgba(239,68,68,0.2); color:var(--red); }
  .event-tag.hit { background:rgba(239,68,68,0.2); color:var(--red); }
  .event-tag.dead { background:rgba(239,68,68,0.3); color:var(--red); }
  .event-tag.sys { background:rgba(59,130,246,0.2); color:var(--blue); }

  .event-text { color:var(--text); }

  .controls { display:grid; grid-template-columns:1fr 1fr 1fr; gap:12px; }

  .btn {
    font-family:'Share Tech Mono',monospace;
    font-size:12px; letter-spacing:2px; text-transform:uppercase;
    padding:12px 16px; border:1px solid var(--border); border-radius:6px;
    background:var(--surface); color:var(--text); cursor:pointer;
    transition:all 0.2s;
  }

  .btn:hover { background:var(--surface2); border-color:var(--accent); color:var(--accent); }
  .btn:active { transform:scale(0.97); }
  .btn.start { border-color:var(--green); color:var(--green); }
  .btn.start:hover { background:rgba(34,197,94,0.1); }
  .btn.end { border-color:var(--red); color:var(--red); }
  .btn.end:hover { background:rgba(239,68,68,0.1); }
  .btn.reset { border-color:var(--accent); color:var(--accent); }
  .btn.reset:hover { background:rgba(245,158,11,0.1); }

  .mode-badge {
    display:inline-block; margin-top:16px; padding:6px 12px;
    border:1px solid var(--green); border-radius:4px;
    font-family:'Share Tech Mono',monospace; font-size:10px;
    color:var(--green); letter-spacing:1px; text-align:center; width:100%;
  }

  .event-log::-webkit-scrollbar { width:4px; }
  .event-log::-webkit-scrollbar-track { background:transparent; }
  .event-log::-webkit-scrollbar-thumb { background:var(--border); border-radius:2px; }

  @media (max-width:650px) {
    .players { grid-template-columns:1fr; }
    .match-timer { font-size:52px; }
    .controls { grid-template-columns:1fr; }
  }
</style>
</head>
<body>
<div class="console">

  <div class="header">
    <h1>FriendlyFire Console</h1>
    <div class="connection-status">
      <div class="status-dot" id="statusDot"></div>
      <span id="statusText">Disconnected</span>
    </div>
  </div>

  <div class="timer-section">
    <div class="match-timer" id="timer">00:00</div>
    <div class="match-status" id="matchStatus">Waiting for match</div>
  </div>

  <div class="players">
    <!-- P1 Blaster -->
    <div class="player-card" id="p1Card">
      <div class="player-name">Player 1 — <span class="tag">Blaster</span></div>
      <div class="stats-row tri">
        <div class="stat-box">
          <div class="stat-label">Ammo</div>
          <div class="stat-value ammo" id="p1Ammo">12</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Shots</div>
          <div class="stat-value shots" id="p1Shots">0</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Reloads</div>
          <div class="stat-value reloads" id="p1Reloads">0</div>
        </div>
      </div>
      <div class="bar-container">
        <div class="bar-fill ammo-bar" id="p1AmmoBar" style="width:100%"></div>
      </div>
      <div class="player-status" id="p1Status">WAITING</div>
    </div>

    <!-- P2 Vest -->
    <div class="player-card" id="p2Card">
      <div class="player-name">Player 2 — <span class="tag vest">Vest</span></div>
      <div class="stats-row">
        <div class="stat-box">
          <div class="stat-label">Lives</div>
          <div class="stat-value lives" id="p2Lives">3</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Hits Taken</div>
          <div class="stat-value hits-taken" id="p2Hits">0</div>
        </div>
      </div>
      <div class="bar-container">
        <div class="bar-fill health-bar" id="p2HealthBar" style="width:100%"></div>
      </div>
      <div class="player-status" id="p2Status">WAITING</div>
    </div>
  </div>

  <div class="event-log" id="eventLog">
    <h2>Event Log</h2>
  </div>

  <div class="controls">
    <button class="btn start" onclick="startTimer()">Start</button>
    <button class="btn end" onclick="endMatch()">End</button>
    <button class="btn reset" onclick="resetAll()">Reset</button>
  </div>

  <div class="mode-badge">ESP32 Hub — Blaster UART + Vest Wi-Fi Link</div>
</div>

<script>
  var ws;
  var matchSeconds = 0;
  var timerInterval = null;
  var timerRunning = false;

  var MAX_AMMO = 12;
  var MAX_LIVES = 3;
  var lastAmmo = 12;
  var totalShots = 0;
  var totalReloads = 0;
  var totalHits = 0;

  function connectWebSocket() {
    var host = window.location.hostname;
    ws = new WebSocket('ws://' + host + ':81');

    ws.onopen = function() {
      document.getElementById('statusDot').className = 'status-dot connected';
      document.getElementById('statusText').textContent = 'Connected';
      addEvent('sys', 'Connected to hub');
    };

    ws.onclose = function() {
      document.getElementById('statusDot').className = 'status-dot';
      document.getElementById('statusText').textContent = 'Disconnected';
      setTimeout(connectWebSocket, 2000);
    };

    ws.onerror = function() { ws.close(); };
    ws.onmessage = function(evt) { handleMessage(evt.data); };
  }

  function handleMessage(raw) {
    try {
      var data = JSON.parse(raw);
      var player = data.player;

      if (player === 'P1') {
        handleBlaster(data);
      } else if (player === 'P2') {
        handleVest(data);
      }
    } catch(e) {
      console.log('Parse error:', raw);
    }
  }

  function handleBlaster(data) {
    var ammo = data.ammo;
    var event = data.event;
    var card = document.getElementById('p1Card');
    var status = document.getElementById('p1Status');

    // Update ammo
    document.getElementById('p1Ammo').textContent = ammo;
    document.getElementById('p1AmmoBar').style.width = (ammo / MAX_AMMO * 100) + '%';

    // Ammo colors
    var ammoEl = document.getElementById('p1Ammo');
    var barEl = document.getElementById('p1AmmoBar');
    ammoEl.className = 'stat-value ammo';
    barEl.className = 'bar-fill ammo-bar';
    if (ammo <= 0) { ammoEl.classList.add('empty'); barEl.classList.add('empty'); }
    else if (ammo <= 3) { ammoEl.classList.add('low'); barEl.classList.add('low'); }

    if (event === 'AMMO' && ammo < lastAmmo) {
      totalShots++;
      document.getElementById('p1Shots').textContent = totalShots;
      card.className = 'player-card flash-shot';
      status.textContent = 'SHOT FIRED';
      status.className = 'player-status shooting';
      setTimeout(function() { card.className = 'player-card'; status.textContent = 'READY'; status.className = 'player-status'; }, 400);
      addEvent('shot', 'P1 fired — Ammo: ' + ammo + '/' + MAX_AMMO);
    }
    else if (event === 'RELOAD') {
      totalReloads++;
      document.getElementById('p1Reloads').textContent = totalReloads;
      card.className = 'player-card flash-reload';
      status.textContent = 'RELOADED';
      status.className = 'player-status reloading';
      setTimeout(function() { card.className = 'player-card'; status.textContent = 'READY'; status.className = 'player-status'; }, 800);
      addEvent('reload', 'P1 reloaded — Ammo: ' + ammo + '/' + MAX_AMMO);
    }
    else if (event === 'EMPTY') {
      card.className = 'player-card flash-empty';
      status.textContent = 'NO AMMO';
      status.className = 'player-status empty-mag';
      setTimeout(function() { card.className = 'player-card'; }, 400);
      addEvent('empty', 'P1 trigger — magazine empty');
    }
    else {
      status.textContent = 'READY';
      status.className = 'player-status';
      addEvent('sys', 'P1 online — ' + ammo + ' rounds');
    }

    lastAmmo = ammo;
  }

  function handleVest(data) {
    var lives = data.lives;
    var event = data.event;
    var card = document.getElementById('p2Card');
    var status = document.getElementById('p2Status');

    // Update lives
    document.getElementById('p2Lives').textContent = lives;
    document.getElementById('p2HealthBar').style.width = (lives / MAX_LIVES * 100) + '%';

    // Lives colors
    var livesEl = document.getElementById('p2Lives');
    var barEl = document.getElementById('p2HealthBar');
    livesEl.className = 'stat-value lives';
    barEl.className = 'bar-fill health-bar';
    if (lives <= 0) { livesEl.classList.add('critical'); barEl.classList.add('critical'); }
    else if (lives <= 1) { livesEl.classList.add('low'); barEl.classList.add('low'); }

    if (event === 'HIT') {
      totalHits++;
      document.getElementById('p2Hits').textContent = totalHits;
      card.className = 'player-card flash-hit';
      status.textContent = 'HIT!';
      status.className = 'player-status hit-status';
      setTimeout(function() { card.className = 'player-card'; status.textContent = 'ALIVE'; status.className = 'player-status'; }, 600);
      addEvent('hit', 'P2 hit! Lives: ' + lives + '/' + MAX_LIVES);
    }
    else if (event === 'DEAD') {
      card.className = 'player-card flash-dead';
      card.classList.add('eliminated');
      status.textContent = 'ELIMINATED';
      status.className = 'player-status dead-status';
      addEvent('dead', 'P2 eliminated!');
      endMatch();
    }
    else if (event === 'HEALTH') {
      card.className = 'player-card';
      card.classList.remove('eliminated');
      status.textContent = 'ALIVE';
      status.className = 'player-status';
      addEvent('sys', 'P2 online — ' + lives + ' lives');
    }
    else {
      addEvent('sys', 'P2 ' + event + ' — ' + lives);
    }
  }

  function startTimer() {
    if (!timerRunning) {
      timerRunning = true;
      matchSeconds = 0;
      document.getElementById('matchStatus').textContent = 'Match in progress';
      document.getElementById('matchStatus').className = 'match-status active';
      timerInterval = setInterval(function() {
        matchSeconds++;
        document.getElementById('timer').textContent = formatTime(matchSeconds);
      }, 1000);
      addEvent('sys', 'Match started');
    }
  }

  function endMatch() {
    if (timerRunning) {
      timerRunning = false;
      clearInterval(timerInterval);
      document.getElementById('matchStatus').textContent = 'Match ended';
      document.getElementById('matchStatus').className = 'match-status ended';
      addEvent('sys', 'Match ended');
    }
  }

  function resetAll() {
    timerRunning = false;
    clearInterval(timerInterval);
    matchSeconds = 0;
    totalShots = 0;
    totalReloads = 0;
    totalHits = 0;
    lastAmmo = MAX_AMMO;

    document.getElementById('timer').textContent = '00:00';
    document.getElementById('matchStatus').textContent = 'Waiting for match';
    document.getElementById('matchStatus').className = 'match-status';

    document.getElementById('p1Ammo').textContent = MAX_AMMO;
    document.getElementById('p1Ammo').className = 'stat-value ammo';
    document.getElementById('p1AmmoBar').style.width = '100%';
    document.getElementById('p1AmmoBar').className = 'bar-fill ammo-bar';
    document.getElementById('p1Shots').textContent = '0';
    document.getElementById('p1Reloads').textContent = '0';
    document.getElementById('p1Status').textContent = 'WAITING';
    document.getElementById('p1Status').className = 'player-status';
    document.getElementById('p1Card').className = 'player-card';

    document.getElementById('p2Lives').textContent = MAX_LIVES;
    document.getElementById('p2Lives').className = 'stat-value lives';
    document.getElementById('p2HealthBar').style.width = '100%';
    document.getElementById('p2HealthBar').className = 'bar-fill health-bar';
    document.getElementById('p2Hits').textContent = '0';
    document.getElementById('p2Status').textContent = 'WAITING';
    document.getElementById('p2Status').className = 'player-status';
    document.getElementById('p2Card').className = 'player-card';

    var log = document.getElementById('eventLog');
    var events = log.querySelectorAll('.event');
    for (var i = 0; i < events.length; i++) { events[i].remove(); }

    addEvent('sys', 'System reset');

    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send('RESET');
    }
  }

  function formatTime(s) {
    var m = Math.floor(s / 60);
    var sec = s % 60;
    return (m < 10 ? '0' : '') + m + ':' + (sec < 10 ? '0' : '') + sec;
  }

  function addEvent(tag, text) {
    var log = document.getElementById('eventLog');
    var el = document.createElement('div');
    el.className = 'event';
    el.innerHTML =
      '<span class="event-time">' + formatTime(matchSeconds) + '</span>' +
      '<span class="event-tag ' + tag + '">' + tag.toUpperCase() + '</span>' +
      '<span class="event-text">' + text + '</span>';
    var h2 = log.querySelector('h2');
    h2.insertAdjacentElement('afterend', el);
    log.scrollTop = 0;
  }

  connectWebSocket();
</script>
</body>
</html>
)rawliteral";

// ===== Broadcast JSON to all WebSocket clients =====
void broadcastJSON(String json) {
  webSocket.broadcastTXT(json);
  Serial.println("[TX] " + json);
}

// ===== Parse blaster UART: P1,EVENT,AMMO =====
void handleBlasterLine(const String& line) {
  int c1 = line.indexOf(',');
  int c2 = line.indexOf(',', c1 + 1);

  if (c1 < 0 || c2 < 0) {
    Serial.println("[UART] Bad format: " + line);
    return;
  }

  String player = line.substring(0, c1);
  String event  = line.substring(c1 + 1, c2);
  int ammo      = line.substring(c2 + 1).toInt();

  if (player != "P1") {
    Serial.println("[UART] Unknown: " + line);
    return;
  }

  Serial.printf("[UART] P1 %s ammo=%d\n", event.c_str(), ammo);

  String json = "{\"player\":\"P1\",\"event\":\"" + event + "\",\"ammo\":" + String(ammo) + "}";
  broadcastJSON(json);
}

// ===== Poll blaster UART =====
void pollBlasterUART() {
  while (Serial1.available() > 0) {
    char ch = (char)Serial1.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      if (blasterLine.length() > 0) {
        handleBlasterLine(blasterLine);
        blasterLine = "";
      }
      continue;
    }
    blasterLine += ch;
    if (blasterLine.length() > 48) {
      Serial.println("[UART] Overflow, dropping");
      blasterLine = "";
    }
  }
}

// ===== WebSocket event handler =====
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %u disconnected\n", num);
      break;

    case WStype_CONNECTED:
      Serial.printf("[WS] Client %u connected\n", num);
      break;

    case WStype_TEXT: {
      String msg = String((char*)payload);
      Serial.printf("[WS] Client %u: %s\n", num, msg.c_str());

      // Browser sent RESET — forward to all clients (vest feather listens)
      if (msg == "RESET") {
        webSocket.broadcastTXT("VEST_RESET");
        Serial.println("[CMD] Broadcasting VEST_RESET");
      }
      // Vest feather sent JSON — forward to all clients (browser picks it up)
      else if (msg.startsWith("{")) {
        broadcastJSON(msg);
      }
      break;
    }
  }
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== FriendlyFire Hub (Final Demo) ===");

  // Blaster UART — 9600 baud, 8N2
  Serial1.begin(9600, SERIAL_8N2, blasterRxPin, blasterTxPin);
  Serial.println("Blaster UART on RX pin " + String(blasterRxPin));

  // Wi-Fi AP
  WiFi.softAP("FriendlyFire", "lasertag1");
  delay(1000);
  Serial.print("AP: FriendlyFire  Pass: lasertag1  Open: ");
  Serial.println(WiFi.softAPIP());

  // HTTP
  server.on("/", []() {
    server.send_P(200, "text/html", webpage);
  });
  server.begin();
  Serial.println("HTTP on :80");

  // WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket on :81");
}

// ===== Loop =====
void loop() {
  server.handleClient();
  webSocket.loop();
  pollBlasterUART();
}
