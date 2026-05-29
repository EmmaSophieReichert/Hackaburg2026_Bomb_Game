// Bomb Defusal — ESP8266 standalone (kein Pi).
// Portierung von bomb-esp32/bomb-esp32.ino — gleiche Spiellogik, gleiches
// Frontend (index_html.h), gleicher WebSocket-JSON-Datenvertrag.
// Unterschiede zur ESP32-Version: ESP8266-WiFi-/AsyncTCP-Header und
// ESP8266-taugliche Pin-Belegung (GPIO 18/19/21/22/23 existieren hier nicht).
//
// Benoetigte Libraries (Arduino IDE -> Library Manager):
//   - "ESPAsyncWebServer"  (me-no-dev / ESP32Async, ESP8266-tauglich)
//   - "ESPAsyncTCP"        (ESP8266-Variante von AsyncTCP)
// Board: ein beliebiges ESP8266-Dev-Board (NodeMCU / Wemos D1 mini).

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "index_html.h"
#include "app_js.h"

// ---------- Access Point ----------
const char* AP_SSID = "BOMB-DEFUSAL";
const char* AP_PASS = "defuse123";        // min. 8 Zeichen; "" = offenes Netz

// ---------- Pins (ESP8266, 3.3V!) ----------
// TRIG ist Ausgang -> GPIO16 (D0) ist ok. ECHO/Draehte nur auf Pins ohne
// Boot-Zwaenge: GPIO 4,5,12,13,14 (D2,D1,D6,D7,D5). Alle Draht-Pins koennen
// internen Pull-up.
const int PIN_TRIG = 16;                   // HC-SR04 Trigger (D0)
const int PIN_ECHO = 4;                    // HC-SR04 Echo (D2) -> SPANNUNGSTEILER auf 3.3V!
const int WIRE_PIN[4] = {5, 14, 12, 13};   // rot(D1), blau(D5), gruen(D6), gelb(D7) -> gegen GND
const char* WIRE_NAME[4] = {"rot", "blau", "gruen", "gelb"};
const int CORRECT_WIRE = 1;                // Index 1 = "blau"

// ---------- Spielkonfiguration ----------
const float GAME_TIME = 150.0;             // Sekunden bis Boom
const char* WIRE_HINT = "Schneide nicht Rot. Die Loesung ist kuehl wie das Meer.";

// ---------- Minispiele ----------
struct Stage {
  const char* title;
  const char* instruction;
  float zmin, zmax, hold_s, scale_max, limit_s;   // limit_s: Zeitbudget; abgelaufen = Level gefailed
};
Stage STAGES[] = {
  {"Annaeherung", "Halte die Hand 3 s ruhig in 10-15 cm vor den Sensor.", 10, 15, 3.0, 40, 30},
  {"Rueckzug",    "Jetzt weiter weg: Hand 3 s in 25-35 cm halten.",       25, 35, 3.0, 40, 30},
};
const int NUM_STAGES = sizeof(STAGES) / sizeof(STAGES[0]);

// ---------- Zustand ----------
enum Phase { IDLE, STAGE, WIRE, DEFUSED, EXPLODED };
Phase phase = IDLE;
int   idx = 0;
unsigned long tEnd = 0;          // millis bei Boom; 0 = nicht gestartet
unsigned long holdStart = 0;     // millis seit in Zone; 0 = nicht in Zone
unsigned long stageStart = 0;    // millis-Start des aktuellen Levels; 0 = nicht gestartet
float held = 0, curD = 0, progress = 0;
String message = "Druecke START zum Scharfschalten.";
String pendingEvent = "";        // one-shot: level_passed|level_failed|defused|exploded
int    pendingLevel = 0;         // 1-basierte Level-Nummer zum Event
bool wireBaseline[4];

void emitEvent(const char* type, int level) { pendingEvent = type; pendingLevel = level; }

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------- Hardware ----------
float readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long dur = pulseIn(PIN_ECHO, HIGH, 30000);   // Timeout 30 ms (~5 m)
  if (dur == 0) return 999.0;                  // ausser Reichweite
  return dur * 0.0343 / 2.0;
}
bool wireIntact(int i) { return digitalRead(WIRE_PIN[i]) == LOW; }  // mit GND verbunden

// ---------- Spiel-Logik (Spiegel von server.py) ----------
float timeLeft() {
  if (tEnd == 0) return GAME_TIME;
  long rem = (long)(tEnd - millis());
  return rem > 0 ? rem / 1000.0 : 0.0;
}

void resetGame() {
  phase = IDLE; idx = 0; tEnd = 0; holdStart = 0; stageStart = 0; held = 0; curD = 0; progress = 0;
  pendingEvent = ""; pendingLevel = 0;
  message = "Druecke START zum Scharfschalten.";
}
void startGame() {
  resetGame();
  phase = STAGE; idx = 0;
  tEnd = millis() + (unsigned long)(GAME_TIME * 1000);
  stageStart = millis();
}
void enterWire() {
  phase = WIRE;
  message = "Letzte Phase: entschaerfe die Bombe.";
  for (int i = 0; i < 4; i++) wireBaseline[i] = wireIntact(i);
}
void advanceStage() {
  int passed = idx + 1;            // gerade abgeschlossenes Level
  idx++;
  holdStart = 0; held = 0; progress = 0;
  emitEvent("level_passed", passed);
  if (idx >= NUM_STAGES) enterWire();
  else { stageStart = millis(); message = "Stufe geschafft. Naechste Aufgabe."; }
}

void stepStage() {
  Stage& st = STAGES[idx];
  float elapsed = stageStart ? (millis() - stageStart) / 1000.0 : 0.0;
  if (elapsed > st.limit_s) {
    emitEvent("level_failed", idx + 1);
    phase = EXPLODED;
    message = "BOOM. Level " + String(idx + 1) + " nicht rechtzeitig geschafft.";
    return;
  }
  int rem = (int)(st.limit_s - elapsed);
  curD = readDistanceCm();
  bool inZone = curD >= st.zmin && curD <= st.zmax;
  if (inZone) {
    if (holdStart == 0) holdStart = millis();
    held = (millis() - holdStart) / 1000.0;
    progress = min(held / st.hold_s, 1.0f);
    message = "Halten... " + String(held, 1) + "/" + String((int)st.hold_s) + "s  ·  noch " + String(rem) + "s";
    if (held >= st.hold_s) advanceStage();
  } else {
    holdStart = 0; held = 0; progress = 0;
    message = "Bring dich in die Zielzone.  ·  noch " + String(rem) + "s";
  }
}

void stepWire() {
  for (int i = 0; i < 4; i++) {
    if (wireBaseline[i] && !wireIntact(i)) {
      if (i == CORRECT_WIRE) { emitEvent("defused", NUM_STAGES + 1); phase = DEFUSED; message = "ENTSCHAERFT. Gut gemacht."; }
      else { emitEvent("exploded", NUM_STAGES + 1); phase = EXPLODED; message = "BOOM. " + String(WIRE_NAME[i]) + " war falsch."; }
      return;
    }
  }
}

void step() {
  if ((phase == STAGE || phase == WIRE) && timeLeft() <= 0) {
    emitEvent("exploded", phase == WIRE ? NUM_STAGES + 1 : idx + 1);
    phase = EXPLODED; message = "BOOM. Zeit abgelaufen."; return;
  }
  if (phase == STAGE) stepStage();
  else if (phase == WIRE) stepWire();
}

// ---------- JSON-Datenvertrag (identisch zu Game.to_dict) ----------
const char* phaseStr() {
  switch (phase) {
    case IDLE: return "IDLE"; case STAGE: return "STAGE"; case WIRE: return "WIRE";
    case DEFUSED: return "DEFUSED"; default: return "EXPLODED";
  }
}
String buildState() {
  bool inStage = (phase == STAGE);
  int stageIndex = inStage ? idx + 1 : NUM_STAGES;
  String title = inStage ? STAGES[idx].title : (phase == WIRE ? "Drahtbank" : "");
  String instr = inStage ? STAGES[idx].instruction : "";

  String j = "{";
  j += "\"phase\":\"" + String(phaseStr()) + "\",";
  j += "\"time_left\":" + String(timeLeft(), 1) + ",";
  j += "\"stage_index\":" + String(stageIndex) + ",";
  j += "\"stage_total\":" + String(NUM_STAGES) + ",";
  j += "\"title\":\"" + title + "\",";
  j += "\"instruction\":\"" + instr + "\",";
  j += "\"progress\":" + String(inStage ? progress : 0.0, 3) + ",";
  if (inStage) {
    Stage& st = STAGES[idx];
    j += "\"gauge\":{\"value\":" + String(curD, 1) + ",\"min\":" + String(st.zmin, 0)
       + ",\"max\":" + String(st.zmax, 0) + ",\"scale_max\":" + String(st.scale_max, 0) + "},";
  } else {
    j += "\"gauge\":null,";
  }
  j += "\"message\":\"" + message + "\",";
  j += "\"hint\":\"" + String(phase == WIRE ? WIRE_HINT : "") + "\",";
  j += "\"event\":\"" + pendingEvent + "\",";
  j += "\"event_level\":" + String(pendingLevel) + ",";
  j += "\"wires\":[\"rot\",\"blau\",\"gruen\",\"gelb\"]";
  j += "}";
  return j;
}

// ---------- WebSocket ----------
void onWsEvent(AsyncWebSocket* s, AsyncWebSocketClient* client, AwsEventType type,
               void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    client->text(buildState());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String msg;
      for (size_t i = 0; i < len; i++) msg += (char)data[i];
      if (msg.indexOf("start") >= 0) startGame();
      else if (msg.indexOf("reset") >= 0) resetGame();
    }
  }
}

// ---------- Setup / Loop ----------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  for (int i = 0; i < 4; i++) pinMode(WIRE_PIN[i], INPUT_PULLUP);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());  // i.d.R. 192.168.4.1

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "application/javascript; charset=utf-8", APP_JS);
  });
  server.begin();
}

unsigned long lastTick = 0;
void loop() {
  unsigned long now = millis();
  if (now - lastTick >= 66) {   // ~15 Hz
    lastTick = now;
    step();
    ws.textAll(buildState());
    pendingEvent = ""; pendingLevel = 0;   // Event ist one-shot
  }
  ws.cleanupClients();
}
