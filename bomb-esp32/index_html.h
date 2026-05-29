#pragma once
#include <pgmspace.h>

// AUTOGENERIERT von sync_frontend.py aus frontend/index.html — NICHT direkt editieren.
// Frontend-Aenderungen in frontend/index.html machen und das Skript erneut laufen lassen.
const char INDEX_HTML[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<!--
  Bomb Defusal — Frontend (Markup + CSS). Single source, served by server.py
  and the ESP sketches at "/". Logic lives in app.js. index_html.h is
  generated from this file via sync_frontend.py — edit here, never the header.
-->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Bomb Defusal — Console</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Newsreader:ital,opsz,wght@0,6..72,400;0,6..72,500;1,6..72,400&family=Plus+Jakarta+Sans:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500;700&display=swap" rel="stylesheet">
<style>
:root {
  --bg: #060a10;
  --panel-a: #141b27;
  --panel-b: #0c111a;
  --shell: rgba(255,255,255,0.025);
  --face: #11161f;
  --ink: #e6edf3;
  --muted: #8b97a8;
  --faint: #5c6677;
  --hair: rgba(230,237,243,0.08);
  --line-2: #232c3b;
  --accent: #58a6ff;
  --pink: #ff5d8f;
  --green: #7ee787;
  --purple: #a371f7;
  --yellow: #ffd166;
  --red: #ff5d5d;
  --serif: "Newsreader", Georgia, serif;
  --sans: "Plus Jakarta Sans", system-ui, sans-serif;
  --mono: "JetBrains Mono", "SF Mono", monospace;
  --spring: cubic-bezier(0.32, 0.72, 0, 1);
  --ease: cubic-bezier(0.16, 1, 0.3, 1);
  --r-out: 26px;
  --r-in: 19px;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
html { -webkit-text-size-adjust: 100%; }
body {
  background: var(--bg); color: var(--ink); font-family: var(--sans);
  font-size: 14px; line-height: 1.6; -webkit-font-smoothing: antialiased;
  overflow-x: hidden; min-height: 100dvh; display: flex; flex-direction: column;
}
.ambient {
  position: fixed; inset: 0; pointer-events: none; z-index: 0;
  background:
    radial-gradient(680px 460px at 14% 4%, rgba(88,166,255,0.10), transparent 70%),
    radial-gradient(720px 480px at 90% 0%, rgba(163,113,247,0.09), transparent 72%),
    radial-gradient(760px 560px at 74% 100%, rgba(255,93,143,0.07), transparent 74%);
}
.grain {
  position: fixed; inset: 0; z-index: 50; pointer-events: none; opacity: 0.035;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='120' height='120'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.9' numOctaves='3'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)'/%3E%3C/svg%3E");
}

/* ---- floating glass nav ---- */
.nav-wrap { position: relative; z-index: 4; display: flex; justify-content: center; padding: 22px 16px 0; }
.nav {
  display: flex; align-items: center; gap: 16px; width: max-content; max-width: 100%;
  padding: 9px 9px 9px 16px; border-radius: 999px;
  background: rgba(20,27,39,0.55); border: 1px solid var(--hair);
  backdrop-filter: blur(18px); -webkit-backdrop-filter: blur(18px);
  box-shadow: inset 0 1px 0 rgba(255,255,255,0.06), 0 24px 60px -32px rgba(0,0,0,0.9);
}
.brand { display: flex; align-items: center; gap: 11px; }
.brand-mark {
  width: 34px; height: 34px; display: grid; place-items: center; border-radius: 11px;
  background: var(--face); border: 1px solid var(--line-2);
  box-shadow: inset 0 1px 0 rgba(255,255,255,0.06);
}
.brand-mark svg { width: 20px; height: 20px; }
.brand-text { display: flex; flex-direction: column; line-height: 1.15; }
.brand-text strong { font-family: var(--serif); font-weight: 500; font-size: 17px; letter-spacing: -0.01em; }
.brand-text em { font-style: italic; font-family: var(--serif); color: var(--muted); font-size: 11px; }
.conn {
  display: flex; align-items: center; gap: 8px; padding: 7px 14px 7px 12px; border-radius: 999px;
  background: rgba(255,255,255,0.03); border: 1px solid var(--hair);
  font-family: var(--mono); font-size: 11px; color: var(--muted); letter-spacing: 0.04em;
}
.conn-dot { width: 8px; height: 8px; border-radius: 50%; background: var(--faint); transition: background 0.4s var(--ease), box-shadow 0.4s var(--ease); }
.conn.live .conn-dot { background: var(--green); box-shadow: 0 0 10px var(--green); }
.conn.dead .conn-dot { background: var(--red); box-shadow: 0 0 10px var(--red); }
.mode-toggle {
  display: inline-flex; align-items: center; gap: 7px; padding: 7px 12px; border-radius: 999px;
  background: rgba(255,255,255,0.03); border: 1px solid var(--hair);
  font-family: var(--mono); font-size: 10.5px; color: var(--muted); letter-spacing: 0.12em;
  cursor: pointer; transition: color 0.3s var(--ease), border-color 0.3s var(--ease), background 0.3s var(--ease);
}
.mode-toggle:hover { background: rgba(255,255,255,0.055); color: var(--ink); }
body.demo-mode .mode-toggle { color: var(--yellow); border-color: rgba(255,209,102,0.38); background: rgba(255,209,102,0.08); }

main { position: relative; z-index: 1; flex: 1; display: grid; place-items: center; padding: 56px 18px 80px; }
.console { width: 100%; max-width: 540px; display: flex; flex-direction: column; gap: 22px; }

/* ---- eyebrow ---- */
.eyebrow {
  display: inline-flex; align-items: center; gap: 7px; align-self: flex-start;
  padding: 6px 13px; border-radius: 999px; background: rgba(255,93,143,0.08);
  border: 1px solid rgba(255,93,143,0.28);
  font-size: 10px; font-weight: 600; letter-spacing: 0.22em; text-transform: uppercase; color: #ffb9cf;
}
.eyebrow .ping { width: 6px; height: 6px; border-radius: 50%; background: var(--pink); box-shadow: 0 0 8px var(--pink); }

/* ---- double-bezel ---- */
.bezel {
  background: var(--shell); border: 1px solid var(--hair); border-radius: var(--r-out);
  padding: 7px; box-shadow: 0 40px 80px -50px rgba(0,0,0,0.95);
}
.panel {
  position: relative; background: linear-gradient(165deg, var(--panel-a), var(--panel-b));
  border-radius: var(--r-in); padding: 26px 24px;
  box-shadow: inset 0 1px 1px rgba(255,255,255,0.07), inset 0 0 0 1px rgba(255,255,255,0.015);
}
.panel-head {
  display: flex; align-items: center; gap: 9px; font-size: 10px; font-weight: 600;
  letter-spacing: 0.2em; text-transform: uppercase; color: var(--muted); margin-bottom: 18px;
}
.panel-head .led { width: 6px; height: 6px; border-radius: 50%; background: var(--accent); box-shadow: 0 0 9px var(--accent); }
.panel-sub { margin-left: auto; font-family: var(--serif); font-style: italic; font-size: 11px; color: var(--faint); text-transform: none; letter-spacing: 0; }

/* ---- timer ---- */
.state-row { display: flex; align-items: center; gap: 10px; }
.chip {
  font-family: var(--mono); font-size: 10.5px; letter-spacing: 0.14em; padding: 5px 12px;
  border-radius: 999px; background: rgba(88,166,255,0.1); color: var(--accent);
  border: 1px solid rgba(88,166,255,0.3); transition: all 0.4s var(--ease);
}
.timer {
  font-family: var(--mono); font-weight: 700; font-size: 88px; line-height: 0.95;
  letter-spacing: -0.03em; text-align: center; font-variant-numeric: tabular-nums;
  margin: 14px 0 8px; transition: color 0.4s var(--ease);
}
.timer.warn { color: var(--yellow); }
.timer.danger { color: var(--red); text-shadow: 0 0 30px rgba(255,93,93,0.5); animation: pulse 1s infinite; }
.timer.win { color: var(--green); font-size: 58px; text-shadow: 0 0 30px rgba(126,231,135,0.45); }
.timer.boom { color: var(--red); font-size: 58px; text-shadow: 0 0 34px rgba(255,93,93,0.6); }
@keyframes pulse { 0%,100% { opacity: 1; } 50% { opacity: 0.5; } }
.message { font-family: var(--serif); font-style: italic; font-size: 18px; color: var(--muted); text-align: center; min-height: 28px; }

/* ---- stage tracker ---- */
.stagecount {
  margin-left: auto; font-family: var(--mono); font-size: 10.5px; letter-spacing: 0.14em;
  color: var(--muted); padding: 5px 12px; border-radius: 999px;
  background: rgba(255,255,255,0.03); border: 1px solid var(--hair);
}
.task {
  margin-top: 18px; padding: 16px 18px; border-radius: 14px;
  background: rgba(88,166,255,0.05); border: 1px solid rgba(88,166,255,0.18);
  border-left: 2px solid var(--accent);
}
.task-title { font-family: var(--sans); font-size: 11px; font-weight: 700; letter-spacing: 0.14em; text-transform: uppercase; color: var(--accent); margin-bottom: 5px; }
.task-instr { font-family: var(--serif); font-size: 16px; color: var(--ink); line-height: 1.45; }
.progress { margin-top: 12px; height: 5px; border-radius: 4px; background: rgba(255,255,255,0.06); overflow: hidden; }
.progress-fill { height: 100%; width: 0%; border-radius: 4px; background: linear-gradient(90deg, var(--accent), var(--purple)); transition: width 0.15s var(--spring); }

/* ---- gauge ---- */
.gauge-wrap { margin-top: 18px; }
.gauge-wrap.hidden { display: none; }
.gauge {
  position: relative; width: 100%; height: 48px; border-radius: 14px;
  background: #0a0e15; border: 1px solid var(--line-2); overflow: hidden;
  box-shadow: inset 0 1px 3px rgba(0,0,0,0.6);
}
.gauge-zone {
  position: absolute; top: 0; bottom: 0; background: rgba(126,231,135,0.15);
  border-left: 1px dashed rgba(126,231,135,0.55); border-right: 1px dashed rgba(126,231,135,0.55);
  transition: left 0.5s var(--spring), width 0.5s var(--spring);
}
.gauge-needle {
  position: absolute; top: -4px; bottom: -4px; width: 3px; border-radius: 3px; background: var(--ink);
  box-shadow: 0 0 12px rgba(230,237,243,0.7); transform: translateX(-50%);
  transition: left 0.12s var(--spring), background 0.3s var(--ease), box-shadow 0.3s var(--ease);
}
.gauge-needle.inzone { background: var(--green); box-shadow: 0 0 16px var(--green); }
.gauge-foot { display: flex; justify-content: space-between; margin-top: 10px; font-family: var(--mono); font-size: 11px; color: var(--faint); }
.gauge-foot .dist { color: var(--ink); }

/* ---- hint ---- */
.hint {
  margin-top: 18px; padding: 13px 15px; border-radius: 12px 4px 12px 12px;
  background: linear-gradient(170deg, #ffe7a6, #ffd97a); color: #2c2410;
  font-family: var(--serif); font-style: italic; font-size: 13.5px;
  box-shadow: 0 16px 30px -16px rgba(0,0,0,0.7);
  transform: rotate(-0.8deg) translateY(8px); opacity: 0; pointer-events: none;
  transition: opacity 0.4s var(--ease), transform 0.5s var(--spring);
}
.hint.show { opacity: 0.97; transform: rotate(-0.8deg) translateY(0); pointer-events: auto; }
.hint strong { display: block; font-style: normal; font-family: var(--sans); font-size: 9px; letter-spacing: 0.2em; text-transform: uppercase; margin-bottom: 4px; color: #6b540f; }

/* ---- wires ---- */
.wires { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; }
.wire {
  position: relative; aspect-ratio: 1 / 1.2; border-radius: 14px; border: 1px solid var(--line-2);
  background: linear-gradient(165deg, #131925, #0c111a); display: flex; flex-direction: column;
  align-items: center; justify-content: center; gap: 10px; overflow: hidden;
  box-shadow: inset 0 1px 0 rgba(255,255,255,0.04); transition: border-color 0.4s var(--ease), transform 0.3s var(--spring);
}
.wires.armed .wire { border-color: rgba(255,93,143,0.4); }
.wires.armed .wire:nth-child(odd) { transform: translateY(-2px); }
.wire-strip { width: 56%; height: 8px; border-radius: 5px; box-shadow: 0 0 12px currentColor; }
.wire-label { font-family: var(--mono); font-size: 9.5px; letter-spacing: 0.16em; color: var(--muted); text-transform: uppercase; }
.wire .rot { background: var(--red); color: var(--red); } .wire.lbl-rot .wire-label { color: var(--red); }
.wire .blau { background: var(--accent); color: var(--accent); } .wire.lbl-blau .wire-label { color: var(--accent); }
.wire .gruen { background: var(--green); color: var(--green); } .wire.lbl-gruen .wire-label { color: var(--green); }
.wire .gelb { background: var(--yellow); color: var(--yellow); } .wire.lbl-gelb .wire-label { color: var(--yellow); }

/* ---- CTA: button-in-button ---- */
.toolbar { display: flex; gap: 12px; flex-wrap: wrap; }
.tool {
  flex: 1; display: inline-flex; align-items: center; justify-content: space-between; gap: 12px;
  font-family: var(--sans); font-size: 14px; font-weight: 600; color: var(--ink);
  background: rgba(255,255,255,0.025); border: 1px solid var(--hair);
  padding: 13px 13px 13px 22px; border-radius: 999px; cursor: pointer;
  transition: background 0.5s var(--spring), border-color 0.5s var(--spring), transform 0.18s var(--spring);
}
.tool:hover { background: rgba(255,255,255,0.05); border-color: rgba(255,255,255,0.16); }
.tool:active { transform: scale(0.975); }
.tool .ic {
  width: 34px; height: 34px; flex-shrink: 0; display: grid; place-items: center; border-radius: 50%;
  background: rgba(255,255,255,0.07); transition: transform 0.5s var(--spring), background 0.4s var(--ease);
}
.tool:hover .ic { transform: translate(2px, -1px) scale(1.06); }
.tool svg { width: 16px; height: 16px; }
.tool.primary { background: rgba(255,93,143,0.13); border-color: rgba(255,93,143,0.42); color: #ffd1de; }
.tool.primary:hover { background: rgba(255,93,143,0.2); }
.tool.primary .ic { background: rgba(255,93,143,0.22); }
.tool.demo-tool { display: none; }
body.demo-mode .tool.demo-tool { display: inline-flex; }

/* ---- entry motion ---- */
.reveal { opacity: 0; transform: translateY(22px); filter: blur(6px); }
.reveal.in { opacity: 1; transform: translateY(0); filter: blur(0); transition: opacity 0.8s var(--ease), transform 0.9s var(--spring), filter 0.8s var(--ease); }

/* ---- level event toast ---- */
.toast {
  position: fixed; top: 92px; left: 50%; z-index: 60; pointer-events: none;
  display: flex; align-items: center; gap: 10px;
  padding: 13px 20px 13px 16px; border-radius: 14px;
  font-family: var(--mono); font-size: 13px; font-weight: 500; letter-spacing: 0.04em;
  background: rgba(20,27,39,0.92); border: 1px solid var(--hair); color: var(--ink);
  backdrop-filter: blur(14px); -webkit-backdrop-filter: blur(14px);
  box-shadow: 0 30px 60px -30px rgba(0,0,0,0.95);
  opacity: 0; transform: translate(-50%, -14px) scale(0.96);
  transition: opacity 0.35s var(--ease), transform 0.5s var(--spring);
}
.toast.show { opacity: 1; transform: translate(-50%, 0) scale(1); }
.toast::before { content: ""; width: 9px; height: 9px; border-radius: 50%; flex-shrink: 0; }
.toast.good { border-color: rgba(126,231,135,0.4); }
.toast.good::before { background: var(--green); box-shadow: 0 0 12px var(--green); }
.toast.bad { border-color: rgba(255,93,93,0.45); }
.toast.bad::before { background: var(--red); box-shadow: 0 0 12px var(--red); }

@media (max-width: 600px) {
  .timer { font-size: 68px; }
  .panel { padding: 22px 18px; }
  .brand-text { display: none; }
}
</style>
</head>
<body>
<div class="ambient" aria-hidden="true"></div>
<div class="grain" aria-hidden="true"></div>
<div class="toast" id="toast" role="status" aria-live="polite"></div>

<div class="nav-wrap">
  <nav class="nav reveal" data-delay="0">
    <div class="brand">
      <span class="brand-mark">
        <svg viewBox="0 0 24 24" fill="none">
          <circle cx="11" cy="14" r="6.6" fill="#0c111a" stroke="#ff5d8f" stroke-width="1.3"/>
          <path d="M15.8 9.2l2.6-2.6M18.4 6.6l1.9.6M18.4 6.6l-.6-1.9" stroke="#ffd166" stroke-width="1.3" stroke-linecap="round"/>
          <circle cx="20.6" cy="4.6" r="1" fill="#ff5d5d"/>
        </svg>
      </span>
      <span class="brand-text">
        <strong>Bomb Defusal</strong>
        <em>field console</em>
      </span>
    </div>
    <div class="conn" id="conn">
      <span class="conn-dot"></span><span id="conn-label">connecting...</span>
    </div>
    <button class="mode-toggle" id="mode-toggle" type="button">PROD</button>
  </nav>
</div>

<main>
  <div class="console">
    <span class="eyebrow reveal" data-delay="60"><span class="ping"></span>live detonator</span>

    <div class="bezel reveal" data-delay="120">
      <section class="panel">
        <div class="panel-head"><span class="led"></span>Detonator
          <span class="stagecount" id="stagecount">—</span>
        </div>
        <div class="state-row"><span class="chip" id="chip">IDLE</span></div>
        <div class="timer" id="timer">2:30</div>
        <div class="message" id="message">Press START to arm the device.</div>

        <div class="task" id="task" style="display:none">
          <div class="task-title" id="task-title">Task</div>
          <div class="task-instr" id="task-instr"></div>
          <div class="progress"><div class="progress-fill" id="progress"></div></div>
        </div>

        <div class="hint" id="hint"><strong>Hint</strong><span id="hint-text"></span></div>
      </section>
    </div>

    <div class="bezel reveal" data-delay="200">
      <section class="panel">
        <div class="panel-head"><span class="led"></span>Wire Bank <span class="panel-sub">cut the right one</span></div>
        <div class="wires" id="wires"></div>
      </section>
    </div>

    <div class="toolbar reveal" data-delay="280">
      <button class="tool primary" onclick="send('start')">
        START
        <span class="ic"><svg viewBox="0 0 24 24" fill="none"><path d="M8 5.5v13l10.5-6.5z" fill="currentColor"/></svg></span>
      </button>
      <button class="tool" onclick="send('reset')">
        RESET
        <span class="ic"><svg viewBox="0 0 24 24" fill="none"><path d="M5 12a7 7 0 107-7 7 7 0 00-5.2 2.3M5 4.5V8h3.5" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round"/></svg></span>
      </button>
      <button class="tool demo-tool" onclick="send('advance')">
        DEMO NEXT
        <span class="ic"><svg viewBox="0 0 24 24" fill="none"><path d="M5 5.5v13l9-6.5zM17 6v12" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round"/></svg></span>
      </button>
    </div>
  </div>
</main>

<script src="/app.js"></script>
</body>
</html>
)rawhtml";
