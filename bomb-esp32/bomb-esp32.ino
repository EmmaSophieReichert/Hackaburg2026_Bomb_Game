// Bomb Defusal — ESP32 standalone (kein Pi).
// ESP32 spannt einen eigenen WLAN-Access-Point auf, liefert das Frontend
// (index_html.h) aus und broadcastet den Spielzustand per WebSocket.
// Portiert aus bomb-mvp/server.py — gleicher JSON-Datenvertrag.
//
// Benoetigte Libraries (Arduino IDE -> Library Manager):
//   - "ESP Async WebServer"  (ESP32Async / mathieucarbou Fork)
//   - "Async TCP"            (passende AsyncTCP-Lib fuer ESP32)
// Board: ein beliebiges ESP32-Dev-Board.

#include "app_js.h"
#include "index_html.h"
#include <Adafruit_NeoPixel.h>
#include <AsyncTCP.h>
#include <DHT.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

// ---------- Access Point ----------
const char *AP_SSID = "BOMB-DEFUSAL";
const char *AP_PASS = "defuse123"; // min. 8 Zeichen; "" = offenes Netz

// ---------- Pins (ESP32, 3.3V!) ----------
const int PIN_TRIG = 5;  // HC-SR04 Trigger
const int PIN_ECHO = 18; // HC-SR04 Echo -> SPANNUNGSTEILER auf 3.3V!
// Drahtliste: zum Erweitern einfach Pin + Name anhaengen (z.B. blau=22,
// gruen=23) und ggf. CORRECT_WIRE anpassen. NUM_WIRES und das wires-Feld ziehen
// automatisch mit.
const int WIRE_PIN[] = {21, 19, 23,
                        33}; // rot, gelb, gruen, blau -> jeweils gegen GND
const char *WIRE_NAME[] = {"rot", "gelb", "gruen", "blau"};
const int NUM_WIRES = sizeof(WIRE_PIN) / sizeof(WIRE_PIN[0]);
const int CORRECT_WIRE = 0; // Index 0 = "rot" (gelb wird in Level 1 gezogen)

// Joystick: NUR ADC1-Pins (32-39), da ADC2 bei aktivem WLAN blockiert ist.
// VCC an 3V3 (nicht 5V!), sonst >3,3V am ADC. Achsen 0..4095 (12-bit).
const int PIN_JOY_X = 34;  // VRx -> ADC1
const int PIN_JOY_Y = 35;  // VRy -> ADC1
const int PIN_POT = 32;    // B50K Schleifer -> ADC1 (Level 3)
const int PIN_BUZZER = 14; // NPN-Basis -> 16-Ohm-Lautsprecher (tone)
const int PIN_DHT = 22;    // DHT11 DATA (Level 4). VCC an 3V3, GND an GND.
DHT dht(PIN_DHT, DHT11);
// Externer Hardware-Timer (Arduino Uno) = Master-Uhr:
const int PIN_TIMER_DONE =
    4; // Uno -> ESP: LOW = Zeit abgelaufen. UEBER SPANNUNGSTEILER 5V->3V!
const int PIN_TIMER_RESET =
    17; // ESP -> Uno: kurz LOW pulsen = Timer neu starten. Ruhepegel HIGH.
const int PIN_STRIP = 13; // WS2812B DATA (DIN) -> ~330 Ohm in Reihe
const int NUM_LEDS = 130; // adressierbare Leiste, extern mit 5V/3A versorgt
const uint8_t STRIP_BRIGHTNESS = 100; // ~40% -> Strombudget unter 3A halten
Adafruit_NeoPixel strip(NUM_LEDS, PIN_STRIP, NEO_GRB + NEO_KHZ800);
const int JOY_LOW = 1000; // Ausschlag-Schwellen um die Mitte (~2048)
const int JOY_HIGH = 3000;

// Status-LEDs, parallel zu WIRE_PIN: leuchten solange der Draht intakt ist,
// aus wenn gezogen. Eigene Ausgaenge + Vorwiderstand (220-330 Ohm), Anode an
// den Pin, Kathode an GND. NICHT in die Sense-Leitung des Drahts haengen!
const int LED_PIN[] = {
    25, 26, 27,
    16}; // rot-, gelb-, gruen-, blau-LED (gleiche Reihenfolge wie WIRE_PIN)

// ---------- Spielkonfiguration ----------
const float GAME_TIME = 150.0; // Sekunden bis Boom
const char *WIRE_HINT =
    "One wire is already gone. Cut the red one to finish it.";

// ---------- Minispiele ----------
enum StageKind {
  DISTANCE_HOLD,
  DISTANCE_WIRE_PULL,
  JOYSTICK_SEQUENCE,
  POT_WIRE_PULL,
  HUMIDITY_WIRE_PULL
};
struct Stage {
  StageKind kind;
  const char *title;
  const char *instruction;
  float zmin, zmax, hold_s, scale_max,
      limit_s;     // limit_s: time budget for this level
  int wire;        // fuer DISTANCE_WIRE_PULL
  const char *seq; // fuer JOYSTICK_SEQUENCE: Ziel aus U/D/L/R
};
Stage STAGES[] = {
    {DISTANCE_WIRE_PULL, "Dr. Red's Eyes",
     "This gauge is Dr. Red's eyes, and age has dimmed them - he only sees "
     "sharp at around 25 cm. Bring something into his focus and hold it there "
     "until the yellow path comes clear.",
     20, 30, 0.0, 40, 45, 1, ""},
    {JOYSTICK_SEQUENCE, "The Labyrinth",
     "You are lost in the maze. Steer the only way out - the shortest path - "
     "and walk it step by step. Reach the exit and pull the red wire.",
     0, 0, 0.0, 0, 45, 0, "DLDDRDDRRDLD"},
    {POT_WIRE_PULL, "The Dial",
     "The dial rests easy at the extremes but the charge sits dead center, "
     "near a hundred. Settle it there and the green path opens.",
     15, 150, 0.0, 1858, 45, 2, ""},
    {HUMIDITY_WIRE_PULL, "The Air",
     "The air is too dry for the blue circuit to give. Make it heavy and warm, "
     "the way a breath would, until the wire lets go.",
     70, 100, 0.0, 100, 45, 3, ""},
};
const int NUM_STAGES = sizeof(STAGES) / sizeof(STAGES[0]);

// ---------- Zustand ----------
enum Phase { IDLE, STAGE, WIRE, DEFUSED, EXPLODED };
Phase phase = IDLE;
int idx = 0;
unsigned long tEnd = 0;      // millis bei Boom; 0 = nicht gestartet
unsigned long holdStart = 0; // millis seit in Zone; 0 = nicht in Zone
unsigned long stageStart =
    0; // millis-Start des aktuellen Levels; 0 = nicht gestartet
unsigned long explodeStart =
    0; // millis beim Eintritt in EXPLODED; 0 = nicht explodiert
unsigned long resetPulseEnd =
    0; // bis millis: GPIO17 haelt LOW (Uno-Timer-Reset-Puls)
unsigned long armTime = 0; // millis beim Spielstart; "time-up"-Pin erst danach
                           // beachten (Uno-Reset-Race)
float held = 0, curD = 0, progress = 0;
String message = "Press START to arm the device.";
String pendingEvent =
    "";               // one-shot: level_passed|level_failed|defused|exploded
int pendingLevel = 0; // 1-basierte Level-Nummer zum Event
bool wireBaseline[NUM_WIRES];
bool stageWireBaseline[NUM_WIRES];
String joyInput = "";   // bisher korrekt eingegebene Joystick-Richtungen
bool joyNeutral = true; // true = Stick mittig, bereit fuer naechste Eingabe

void playBoom();
void emitEvent(const char *type, int level) {
  pendingEvent = type;
  pendingLevel = level;
}

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------- Hardware ----------
float readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long dur = pulseIn(PIN_ECHO, HIGH, 30000); // Timeout 30 ms (~5 m)
  if (dur == 0)
    return 999.0; // ausser Reichweite
  return dur * 0.0343 / 2.0;
}
bool wireIntact(int i) {
  return digitalRead(WIRE_PIN[i]) == LOW;
} // mit GND verbunden
int readPot() { // gemittelt gegen ADC-Jitter
  long s = 0;
  for (int i = 0; i < 8; i++)
    s += analogRead(PIN_POT);
  return (int)(s / 8);
}
// DHT11 ist langsam (~1 Hz) und blockierend -> nur alle 2 s lesen, Wert cachen.
float dhtHumidity = 0.0;
unsigned long lastDht = 0;
float readHumidity() {
  if (lastDht == 0 || millis() - lastDht >= 2000) {
    lastDht = millis();
    float h = dht.readHumidity();
    if (!isnan(h))
      dhtHumidity = h; // NaN (Lesefehler) -> alten Wert halten
  }
  return dhtHumidity;
}

// ---------- Spiel-Logik (Spiegel von server.py) ----------
float timeLeft() {
  if (tEnd == 0)
    return GAME_TIME;
  long rem = (long)(tEnd - millis());
  return rem > 0 ? rem / 1000.0 : 0.0;
}

void resetStageRuntime() {
  holdStart = 0;
  held = 0;
  progress = 0;
  joyInput = "";
  joyNeutral = true;
  for (int i = 0; i < NUM_WIRES; i++)
    stageWireBaseline[i] = wireIntact(i);
}

char joyDir() {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  // Joystick ist verdreht: links/rechts passen, oben/unten sind invertiert.
  if (y > JOY_HIGH)
    return 'L';
  if (y < JOY_LOW)
    return 'R';
  if (x > JOY_HIGH)
    return 'D';
  if (x < JOY_LOW)
    return 'U';
  return 0; // mittig
}

String joyWords(const char *seq) {
  String s = "";
  for (int i = 0; seq[i]; i++) {
    if (i)
      s += "-";
    switch (seq[i]) {
    case 'U':
      s += "up";
      break;
    case 'D':
      s += "down";
      break;
    case 'L':
      s += "left";
      break;
    case 'R':
      s += "right";
      break;
    }
  }
  return s;
}

int pulledStageWire() {
  for (int i = 0; i < NUM_WIRES; i++) {
    if (stageWireBaseline[i] && !wireIntact(i))
      return i;
  }
  return -1;
}

void resetGame() {
  phase = IDLE;
  idx = 0;
  tEnd = 0;
  holdStart = 0;
  stageStart = 0;
  explodeStart = 0;
  held = 0;
  curD = 0;
  progress = 0;
  pendingEvent = "";
  pendingLevel = 0;
  message = "Press START to arm the device.";
}
void startGame() {
  resetGame();
  phase = STAGE;
  idx = 0;
  tEnd = millis() + (unsigned long)(GAME_TIME * 1000);
  resetStageRuntime();
  stageStart = millis();
  armTime = millis();
  digitalWrite(PIN_TIMER_RESET,
               LOW); // Uno-Timer neu starten (Puls, in loop() wieder HIGH)
  resetPulseEnd = millis() + 120;
}
void enterWire() {
  phase = WIRE;
  message = "Final phase: defuse the bomb.";
  for (int i = 0; i < NUM_WIRES; i++)
    wireBaseline[i] = wireIntact(i);
}
void advanceStage() {
  int passed = idx + 1; // gerade abgeschlossenes Level
  idx++;
  resetStageRuntime();
  if (idx >= NUM_STAGES) { // alle Raetsel geloest -> entschaerft
    emitEvent("defused", NUM_STAGES + 1);
    phase = DEFUSED;
    message = "DEFUSED. Nice work.";
  } else {
    emitEvent("level_passed", passed);
    stageStart = millis();
    message = "Level cleared. Next task.";
  }
}

void explodeBomb(const char *eventType, int level, const String &boomMessage) {
  emitEvent(eventType, level);
  phase = EXPLODED;
  message = boomMessage;
  if (explodeStart == 0)
    explodeStart = millis();
  playBoom();
}

void demoAdvance() {
  if (phase == STAGE) {
    advanceStage();
  } else if (phase == WIRE) {
    emitEvent("defused", NUM_STAGES + 1);
    phase = DEFUSED;
    message = "DEMO: Bomb defused.";
  }
}

void stepStage() {
  Stage &st = STAGES[idx];
  int rem = (int)timeLeft(); // keine Pro-Level-Limits mehr: nur die globale
                             // (Uno-)Uhr zaehlt

  if (st.kind == JOYSTICK_SEQUENCE) {
    char d = joyDir();
    int total = strlen(st.seq);
    if (d == 0) {
      joyNeutral = true; // zurueck in der Mitte: bereit
    } else if (joyNeutral && (int)joyInput.length() < total) {
      joyNeutral = false; // ein Ausschlag = eine Eingabe
      if (d == st.seq[joyInput.length()])
        joyInput += d; // richtige Richtung
      else
        joyInput = ""; // falsch -> Sequenz zuruecksetzen
    }
    bool done = (int)joyInput.length() >= total;
    int cut = pulledStageWire();
    if (cut >= 0) { // Draht gezogen = abgeben
      if (cut == st.wire && done) {
        advanceStage();
        return;
      }
      explodeBomb("level_failed", idx + 1,
                  (cut != st.wire)
                      ? "BOOM. " + String(WIRE_NAME[cut]) +
                            " was the wrong wire."
                      : "BOOM. The joystick sequence was not complete.");
      return;
    }
    progress = total ? (float)joyInput.length() / total : 0.0;
    message = done ? "Sequence done. Pull the RED wire to confirm.  ·  " +
                         String(rem) + "s left"
                   : "Joystick sequence " + joyWords(st.seq) + "  ·  " +
                         String((int)joyInput.length()) + "/" + String(total) +
                         "  ·  " + String(rem) + "s left";
    return;
  }

  if (st.kind == POT_WIRE_PULL) {
    int pv = readPot();
    bool inZone = pv >= (int)st.zmin && pv <= (int)st.zmax;
    progress = inZone ? 0.5 : 0.0;
    int cut = pulledStageWire();
    if (cut >= 0) {
      if (cut == st.wire && inZone)
        advanceStage();
      else {
        explodeBomb("level_failed", idx + 1,
                    cut == st.wire
                        ? "BOOM. " + String(WIRE_NAME[st.wire]) +
                              " was pulled outside the calibration zone."
                        : "BOOM. " + String(WIRE_NAME[cut]) +
                              " was the wrong wire.");
      }
      return;
    }
    message =
        inZone ? "Reading " + String(pv) +
                     " — locked on 100. Pull the GREEN wire to confirm.  ·  " +
                     String(rem) + "s left"
               : "Reading " + String(pv) + " — " +
                     String(pv < (int)st.zmin ? "turn up toward 100."
                                              : "turn down toward 100.") +
                     "  ·  " + String(rem) + "s left";
    return;
  }

  if (st.kind == HUMIDITY_WIRE_PULL) {
    float h = readHumidity();
    bool inZone = h >= st.zmin && h <= st.zmax;
    progress = inZone ? 0.5 : 0.0;
    int cut = pulledStageWire();
    if (cut >= 0) {
      if (cut == st.wire && inZone)
        advanceStage();
      else {
        explodeBomb("level_failed", idx + 1,
                    cut == st.wire
                        ? "BOOM. " + String(WIRE_NAME[st.wire]) +
                              " was pulled outside the humidity zone."
                        : "BOOM. " + String(WIRE_NAME[cut]) +
                              " was the wrong wire.");
      }
      return;
    }
    message = inZone
                  ? "Humidity in range — pull the BLUE wire to confirm.  ·  " +
                        String(rem) + "s left"
                  : "Humidity " + String(h, 0) +
                        "% — breathe on the sensor to reach " +
                        String((int)st.zmin) + "-" + String((int)st.zmax) +
                        "%.  ·  " + String(rem) + "s left";
    return;
  }

  curD = readDistanceCm();
  bool inZone = curD >= st.zmin && curD <= st.zmax;
  if (st.kind == DISTANCE_WIRE_PULL) {
    progress = inZone ? 0.5 : 0.0;
    int cut = pulledStageWire();
    if (cut >= 0) {
      if (cut == st.wire && inZone)
        advanceStage();
      else {
        explodeBomb("level_failed", idx + 1,
                    cut == st.wire
                        ? "BOOM. " + String(WIRE_NAME[st.wire]) +
                              " was pulled outside the resonance distance."
                        : "BOOM. " + String(WIRE_NAME[cut]) +
                              " was the wrong wire.");
      }
      return;
    }
    String distStr = curD >= 999.0 ? "out of range" : String(curD, 1) + " cm";
    message =
        "Hold the safe distance and pull the matching wire. Current distance " +
        distStr + "  ·  " + String(rem) + "s left";
    return;
  }
  if (inZone) {
    if (holdStart == 0)
      holdStart = millis();
    held = (millis() - holdStart) / 1000.0;
    progress = min(held / st.hold_s, 1.0f);
    message = "Holding... " + String(held, 1) + "/" + String((int)st.hold_s) +
              "s  ·  " + String(rem) + "s left";
    if (held >= st.hold_s)
      advanceStage();
  } else {
    holdStart = 0;
    held = 0;
    progress = 0;
    message = "Move into the target zone.  ·  " + String(rem) + "s left";
  }
}

void stepWire() {
  for (int i = 0; i < NUM_WIRES; i++) {
    if (wireBaseline[i] && !wireIntact(i)) {
      if (i == CORRECT_WIRE) {
        emitEvent("defused", NUM_STAGES + 1);
        phase = DEFUSED;
        message = "DEFUSED. Nice work.";
      } else
        explodeBomb("exploded", NUM_STAGES + 1,
                    "BOOM. " + String(WIRE_NAME[i]) + " was wrong.");
      return;
    }
  }
}

void step() {
  // Master-Uhr ist der externe Uno-Timer: LOW an PIN_TIMER_DONE = Zeit
  // abgelaufen. armTime-Guard: kurz nach Start ignorieren, bis der Uno seinen
  // Reset verarbeitet hat.
  if ((phase == STAGE || phase == WIRE) && millis() - armTime > 1500 &&
      digitalRead(PIN_TIMER_DONE) == LOW) {
    explodeBomb("exploded", phase == WIRE ? NUM_STAGES + 1 : idx + 1,
                "BOOM. Time expired.");
    return;
  }
  if (phase == STAGE)
    stepStage();
  else if (phase == WIRE)
    stepWire();
}

// ---------- JSON-Datenvertrag (identisch zu Game.to_dict) ----------
const char *phaseStr() {
  switch (phase) {
  case IDLE:
    return "IDLE";
  case STAGE:
    return "STAGE";
  case WIRE:
    return "WIRE";
  case DEFUSED:
    return "DEFUSED";
  default:
    return "EXPLODED";
  }
}
String buildState() {
  bool inStage = (phase == STAGE);
  int stageIndex = inStage ? idx + 1 : NUM_STAGES;
  String title =
      inStage ? STAGES[idx].title : (phase == WIRE ? "Wire Bank" : "");
  String instr = inStage ? STAGES[idx].instruction : "";

  String j = "{";
  j += "\"phase\":\"" + String(phaseStr()) + "\",";
  j += "\"time_left\":" + String(timeLeft(), 1) + ",";
  j += "\"stage_index\":" + String(stageIndex) + ",";
  j += "\"stage_total\":" + String(NUM_STAGES) + ",";
  j += "\"title\":\"" + title + "\",";
  j += "\"instruction\":\"" + instr + "\",";
  j += "\"progress\":" + String(inStage ? progress : 0.0, 3) + ",";
  j += "\"message\":\"" + message + "\",";
  j += "\"hint\":\"" + String(phase == WIRE ? WIRE_HINT : "") + "\",";
  j += "\"event\":\"" + pendingEvent + "\",";
  j += "\"event_level\":" + String(pendingLevel) + ",";
  j += "\"wires\":[";
  for (int i = 0; i < NUM_WIRES; i++) {
    if (i)
      j += ",";
    j += "\"" + String(WIRE_NAME[i]) + "\"";
  }
  j += "]";
  j += "}";
  return j;
}

// ---------- WebSocket ----------
void onWsEvent(AsyncWebSocket *s, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    client->text(buildState());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len &&
        info->opcode == WS_TEXT) {
      String msg;
      for (size_t i = 0; i < len; i++)
        msg += (char)data[i];
      if (msg.indexOf("start") >= 0)
        startGame();
      else if (msg.indexOf("reset") >= 0)
        resetGame();
      else if (msg.indexOf("advance") >= 0)
        demoAdvance();
    }
  }
}

// ---------- Setup / Loop ----------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  for (int i = 0; i < NUM_WIRES; i++)
    pinMode(WIRE_PIN[i], INPUT_PULLUP);
  for (int i = 0; i < NUM_WIRES; i++)
    pinMode(LED_PIN[i], OUTPUT);
  pinMode(PIN_TIMER_DONE,
          INPUT); // Pegel kommt vom Spannungsteiler, kein interner Pull-up
  pinMode(PIN_TIMER_RESET, OUTPUT);
  digitalWrite(PIN_TIMER_RESET, HIGH); // Ruhepegel: nicht resetten
  dht.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // i.d.R. 192.168.4.1

  strip.begin();
  strip.setBrightness(STRIP_BRIGHTNESS);
  strip.clear();
  strip.show();

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send_P(200, "application/javascript; charset=utf-8", APP_JS);
  });
  server.on(
      "/echo", HTTP_GET,
      [](AsyncWebServerRequest *req) { // HC-SR04-Diagnose (GPIO18)
        int level = digitalRead(
            PIN_ECHO); // HIGH = ~3.3V, LOW = ~0V (kein echter Spannungswert)
        float d = readDistanceCm();
        String r = "echo_pin=GPIO" + String(PIN_ECHO);
        r += "  level=" + String(level == HIGH ? "HIGH(~3.3V)" : "LOW(~0V)");
        r += "  distance=" +
             (d >= 999.0 ? String("out of range") : String(d, 1) + "cm");
        req->send(200, "text/plain", r);
      });
  server.on("/joy", HTTP_GET,
            [](AsyncWebServerRequest *req) { // Joystick-Diagnose (ADC1)
              int x = analogRead(PIN_JOY_X);
              int y = analogRead(PIN_JOY_Y);
              char d = joyDir();
              String r = "x=" + String(x) + "  y=" + String(y);
              r += "  dir=" + String(d ? String(d) : String("neutral"));
              r += "  (LOW<" + String(JOY_LOW) + " HIGH>" + String(JOY_HIGH) +
                   ")";
              req->send(200, "text/plain", r);
            });
  server.on("/pot", HTTP_GET,
            [](AsyncWebServerRequest *req) { // Poti-Diagnose (GPIO32, ADC1)
              int pv = readPot();
              String r = "pot_pin=GPIO" + String(PIN_POT);
              r += "  raw=" + String(pv);
              r += "  volt=" + String(pv * 3.3 / 4095.0, 2) + "V";
              req->send(200, "text/plain", r);
            });
  server.on("/humidity", HTTP_GET,
            [](AsyncWebServerRequest *req) { // DHT11-Diagnose
              float h = dht.readHumidity();
              float t = dht.readTemperature();
              String r = "raw_humidity=" +
                         (isnan(h) ? String("NaN(read failed)") : String(h, 1));
              r += "  temp=" + (isnan(t) ? String("NaN") : String(t, 1));
              r += "  cached=" + String(dhtHumidity, 1) + "%";
              req->send(200, "text/plain", r);
            });
  server.begin();
}

const unsigned long BLINK_MS =
    5000; // nur 5 s blinken (deckt sich mit playBoom)
const unsigned long BLINK_INTERVAL = 100; // schneller Takt: 100ms an/aus
// true = sichtbarer "AN"-Halbtakt, solange die 5s-Blinkphase laeuft; danach
// dauerhaft aus.
bool explosionOn() {
  if (phase != EXPLODED || explodeStart == 0)
    return false;
  if (millis() - explodeStart >= BLINK_MS)
    return false;
  return (millis() / BLINK_INTERVAL) % 2;
}

void updateLeds() {
  if (phase == EXPLODED) { // alle Status-LEDs blinken im Boom (5s)
    bool on = explosionOn();
    for (int i = 0; i < NUM_WIRES; i++)
      digitalWrite(LED_PIN[i], on ? HIGH : LOW);
    return;
  }
  for (int i = 0; i < NUM_WIRES; i++)
    digitalWrite(LED_PIN[i], wireIntact(i) ? HIGH : LOW);
}

void playBoom() {
  tone(PIN_BUZZER, 1200, 5000);
} // Detonation: 5 s Dauerton, stoppt selbst (deckt sich mit BLINK_MS)

unsigned long lastBeep = 0;
Phase lastSoundPhase = IDLE;
void updateSound() {
  if (phase == STAGE || phase == WIRE) { // Beeps ueber die ganze Zeit: Start
                                         // ~alle 30s, zum Ende immer schneller
    float tl = timeLeft();
    if (tl > 0.0) {
      float frac = tl / GAME_TIME; // 1.0 bei Start -> 0.0 am Ende
      if (frac > 1.0)
        frac = 1.0;
      unsigned long interval =
          (unsigned long)(100 +
                          frac * 29900); // 30000ms bei Start -> 100ms am Ende
      if (millis() - lastBeep >= interval) {
        lastBeep = millis();
        tone(PIN_BUZZER, 1200, 50);
      }
    }
  } else if (phase == EXPLODED) {
    if (lastSoundPhase != EXPLODED)
      playBoom();
  } else if (phase == DEFUSED) {
    if (lastSoundPhase != DEFUSED)
      tone(PIN_BUZZER, 1500, 600); // einmaliger Erfolgston
  } else {
    noTone(PIN_BUZZER);
  }
  lastSoundPhase = phase;
}

void fillStrip(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++)
    strip.setPixelColor(i, r, g, b);
  strip.show();
}

Phase lastStripPhase = IDLE;
bool stripBlinkOn = false;
void updateStrip() {
  if (phase == EXPLODED) { // rot blinken (5s), synchron zu den Status-LEDs
    bool on = explosionOn();
    if (on !=
        stripBlinkOn) { // nur bei Wechsel neu schreiben (auch das Aus nach 5s)
      stripBlinkOn = on;
      fillStrip(on ? 255 : 0, 0, 0);
    }
  } else if (phase == DEFUSED) { // grün dauerhaft
    if (lastStripPhase != DEFUSED)
      fillStrip(0, 255, 0);
  } else { // sonst aus
    if (lastStripPhase == EXPLODED || lastStripPhase == DEFUSED)
      fillStrip(0, 0, 0);
  }
  lastStripPhase = phase;
}

unsigned long lastTick = 0;
void loop() {
  Serial.println(readDistanceCm());
  unsigned long now = millis();
  if (resetPulseEnd && now >= resetPulseEnd) {
    digitalWrite(PIN_TIMER_RESET, HIGH);
    resetPulseEnd = 0;
  }
  if (now - lastTick >= 66) { // ~15 Hz
    lastTick = now;
    step();
    if (phase == EXPLODED && explodeStart == 0)
      explodeStart = millis(); // Boom-Start merken
    updateLeds();
    updateSound();
    updateStrip();
    ws.textAll(buildState());
    pendingEvent = "";
    pendingLevel = 0; // Event ist one-shot
  }
  ws.cleanupClients();
}
