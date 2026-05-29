// Bomb Defusal — Frontend-Logik. WebSocket-Client: empfaengt ~15x/s den
// Spielzustand vom Controller (server.py bzw. ESP), rendert ihn datengetrieben
// (Timer, Aufgabe, Gauge, Draehte, Level-Event-Toast) und sendet start/reset
// zurueck. Wird als /app.js ausgeliefert; Quelle fuer app_js.h
// (sync_frontend.py) — hier editieren, nicht im generierten Header.

const MAX_CM = 40;
let ws, wiresBuilt = false;

function connect() {
  ws = new WebSocket(`ws://${location.host}/ws`);
  ws.onopen = () => setConn(true);
  ws.onclose = () => { setConn(false); setTimeout(connect, 1000); };
  ws.onmessage = (e) => render(JSON.parse(e.data));
}
function setConn(live) {
  const c = document.getElementById("conn");
  c.className = "conn " + (live ? "live" : "dead");
  document.getElementById("conn-label").textContent = live ? "verbunden" : "getrennt";
}
function send(action) { if (ws && ws.readyState === 1) ws.send(JSON.stringify({ action })); }

// one-shot Level-Event vom Controller (kommt genau einen Tick lang im Stream)
const EVENT_MAP = {
  level_passed: ["good", n => `Level ${n} bestanden`],
  level_failed: ["bad",  n => `Level ${n} gefailed`],
  defused:      ["good", () => "Bombe entschaerft"],
  exploded:     ["bad",  () => "Bombe explodiert"],
};
let toastTimer = null;
function showEvent(type, level) {
  const e = EVENT_MAP[type];
  if (!e) return;
  const t = document.getElementById("toast");
  t.textContent = e[1](level);
  t.className = "toast " + e[0] + " show";
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { t.className = "toast " + e[0]; }, 2600);
}

function buildWires(colors) {
  const box = document.getElementById("wires");
  box.innerHTML = "";
  colors.forEach(c => {
    const w = document.createElement("div");
    w.className = "wire lbl-" + c;
    w.innerHTML = `<span class="wire-strip ${c}"></span><span class="wire-label">${c}</span>`;
    box.appendChild(w);
  });
  wiresBuilt = true;
}

function render(s) {
  document.getElementById("chip").textContent = s.phase;
  document.getElementById("message").textContent = s.message;
  document.getElementById("stagecount").textContent =
    (s.phase === "STAGE" || s.phase === "WIRE")
      ? `Aufgabe ${s.stage_index} / ${s.stage_total}` : "—";

  // timer
  const t = document.getElementById("timer");
  const m = Math.floor(s.time_left / 60), sec = Math.floor(s.time_left % 60);
  if (s.phase === "DEFUSED") { t.textContent = "DEFUSED"; t.className = "timer win"; }
  else if (s.phase === "EXPLODED") { t.textContent = "BOOM"; t.className = "timer boom"; }
  else {
    t.textContent = `${m}:${sec.toString().padStart(2, "0")}`;
    t.className = "timer" + (s.time_left < 10 ? " danger" : s.time_left < 30 ? " warn" : "");
  }

  // task card (only during a minigame stage)
  const task = document.getElementById("task");
  if (s.phase === "STAGE" && s.title) {
    task.style.display = "block";
    document.getElementById("task-title").textContent = `Aufgabe ${s.stage_index}: ${s.title}`;
    document.getElementById("task-instr").textContent = s.instruction;
    document.getElementById("progress").style.width = (100 * s.progress) + "%";
  } else {
    task.style.display = "none";
  }

  // gauge (only when the current stage provides one)
  const gw = document.getElementById("gauge-wrap");
  if (s.gauge) {
    gw.classList.remove("hidden");
    const g = s.gauge, max = g.scale_max;
    const zone = document.getElementById("zone");
    zone.style.left = (100 * g.min / max) + "%";
    zone.style.width = (100 * (g.max - g.min) / max) + "%";
    const needle = document.getElementById("needle");
    needle.style.left = Math.min(100, 100 * g.value / max) + "%";
    const inZone = g.value >= g.min && g.value <= g.max;
    needle.className = "gauge-needle" + (inZone ? " inzone" : "");
    document.getElementById("dist").textContent = g.value + " cm";
    document.getElementById("scale-max").textContent = max + " cm";
  } else {
    gw.classList.add("hidden");
  }

  // hint (wire phase)
  const hint = document.getElementById("hint");
  if (s.hint) { hint.classList.add("show"); document.getElementById("hint-text").textContent = s.hint; }
  else hint.classList.remove("show");

  // wires
  if (!wiresBuilt && s.wires) buildWires(s.wires);
  document.getElementById("wires").classList.toggle("armed", s.phase === "WIRE");

  // one-shot Level-Event (Backend setzt es genau einen Broadcast lang)
  if (s.event) showEvent(s.event, s.event_level);
}

// staggered entry reveal
window.addEventListener("load", () => {
  document.querySelectorAll(".reveal").forEach(el => {
    setTimeout(() => el.classList.add("in"), parseInt(el.dataset.delay || 0));
  });
});
connect();
