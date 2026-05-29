// Bomb Defusal — frontend logic. WebSocket client receives controller state
// about 15x/s, renders the timer/task/wires/toasts, and sends actions back.
// Served as /app.js; source for app_js.h via sync_frontend.py.

const MAX_CM = 40;
let ws, wiresBuilt = false;
let demoMode = localStorage.getItem("bomb-demo-mode") === "1";

function connect() {
  ws = new WebSocket(`ws://${location.host}/ws`);
  ws.onopen = () => setConn(true);
  ws.onclose = () => { setConn(false); setTimeout(connect, 1000); };
  ws.onmessage = (e) => {
    let msg;
    try { msg = JSON.parse(e.data); } catch { return; }
    // two schemas: rich game state (has "phase") vs. teammate level ping
    // ({"level":"level1","status":"true"}). Branch on which fields arrived.
    if ("status" in msg && !("phase" in msg)) handleLevelStatus(msg);
    else render(msg);
  };
}
function setConn(live) {
  const c = document.getElementById("conn");
  c.className = "conn " + (live ? "live" : "dead");
  document.getElementById("conn-label").textContent = live ? "connected" : "offline";
}
function send(action) { if (ws && ws.readyState === 1) ws.send(JSON.stringify({ action })); }
function setDemoMode(on) {
  demoMode = on;
  document.body.classList.toggle("demo-mode", demoMode);
  document.getElementById("mode-toggle").textContent = demoMode ? "DEMO" : "PROD";
  localStorage.setItem("bomb-demo-mode", demoMode ? "1" : "0");
}

const EVENT_MAP = {
  level_passed: ["good", n => `Level ${n} cleared`],
  level_failed: ["bad",  n => `Level ${n} failed`],
  defused:      ["good", () => "Bomb defused"],
  exploded:     ["bad",  () => "Bomb exploded"],
};
// Teammate controller schema: {"level":"level1","status":"true"|"false"}.
// It carries no phase/timer/wires, so we surface the ping as a toast and
// reflect it in the chip + message line; the rest of the UI stays idle.
function handleLevelStatus(msg) {
  const n = parseInt(String(msg.level).replace(/\D/g, ""), 10) || 0;
  const ok = msg.status === true || msg.status === "true";
  showEvent(ok ? "level_passed" : "level_failed", n);
  document.getElementById("chip").textContent = ok ? "CLEARED" : "FAILED";
  document.getElementById("message").textContent =
    ok ? `Level ${n} cleared` : `Level ${n} failed`;
}

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
  const labels = { rot: "red", blau: "blue", gruen: "green", gelb: "yellow" };
  box.innerHTML = "";
  // grid adapts to however many wires the controller sends (2 now, more later)
  box.style.gridTemplateColumns = `repeat(${colors.length}, 1fr)`;
  colors.forEach(c => {
    const w = document.createElement("div");
    w.className = "wire lbl-" + c;
    w.innerHTML = `<span class="wire-strip ${c}"></span><span class="wire-label">${labels[c] || c}</span>`;
    box.appendChild(w);
  });
  wiresBuilt = true;
}

function render(s) {
  document.getElementById("chip").textContent = s.phase;
  document.getElementById("message").textContent = s.message;
  document.getElementById("stagecount").textContent =
    (s.phase === "STAGE" || s.phase === "WIRE")
      ? `Task ${s.stage_index} / ${s.stage_total}` : "—";

  // timer
  const t = document.getElementById("timer");
  const m = Math.floor(s.time_left / 60), sec = Math.floor(s.time_left % 60);
  if (s.phase === "DEFUSED") { t.textContent = "DEFUSED"; t.className = "timer win"; }
  else if (s.phase === "EXPLODED") { t.textContent = "BOOM"; t.className = "timer boom"; }
  else {
    t.textContent = `${m}:${sec.toString().padStart(2, "0")}`;
    t.className = "timer" + (s.time_left < 10 ? " danger" : s.time_left < 30 ? " warn" : "");
  }

  const task = document.getElementById("task");
  if (s.phase === "STAGE" && s.title) {
    task.style.display = "block";
    document.getElementById("task-title").textContent = `Task ${s.stage_index}: ${s.title}`;
    document.getElementById("task-instr").textContent = s.instruction;
    document.getElementById("progress").style.width = (100 * s.progress) + "%";
  } else {
    task.style.display = "none";
  }

  const hint = document.getElementById("hint");
  if (s.hint) { hint.classList.add("show"); document.getElementById("hint-text").textContent = s.hint; }
  else hint.classList.remove("show");

  if (!wiresBuilt && s.wires) buildWires(s.wires);
  document.getElementById("wires").classList.toggle("armed", s.phase === "WIRE");

  if (s.event) showEvent(s.event, s.event_level);
}

// staggered entry reveal
window.addEventListener("load", () => {
  document.getElementById("mode-toggle").addEventListener("click", () => setDemoMode(!demoMode));
  setDemoMode(demoMode);
  document.querySelectorAll(".reveal").forEach(el => {
    setTimeout(() => el.classList.add("in"), parseInt(el.dataset.delay || 0));
  });
});
connect();
