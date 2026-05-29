import asyncio
import os
import time

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse

SIM = os.environ.get("SIM") == "1"

# ---------- Globale Konfiguration ----------
GAME_TIME = 150          # Sekunden bis Boom
CORRECT_WIRE = "blau"    # der richtige Draht
WIRE_COLORS = ["rot", "blau", "gruen", "gelb"]
WIRE_HINT = "Schneide nicht Rot. Die Loesung ist kuehl wie das Meer."
TICK = 1 / 15

# ---------- Hardware-Abstraktion ----------
if not SIM:
    from gpiozero import DistanceSensor, Button

    sensor = DistanceSensor(echo=24, trigger=23, max_distance=2.0, queue_len=5)
    WIRE_PINS = {"rot": 17, "blau": 27, "gruen": 22, "gelb": 5}
    wires = {n: Button(p, pull_up=True) for n, p in WIRE_PINS.items()}

    def read_distance_cm():
        return sensor.distance * 100

    def wire_intact(name):
        return wires[name].is_pressed
else:
    _sim = {"dist": 100.0, "cut": set()}

    def read_distance_cm():
        return _sim["dist"]

    def wire_intact(name):
        return name not in _sim["cut"]


# ---------- Minispiel-Basis ----------
class Stage:
    """Ein Minispiel. update() liefert True, wenn geloest."""
    title = ""
    instruction = ""

    def reset(self):
        pass

    def update(self):
        return False

    def progress(self):
        return 0.0

    def gauge(self):
        return None  # optional: {"value","min","max","scale_max"}


class DistanceHold(Stage):
    """Hand fuer hold_s Sekunden in [zmin, zmax] cm halten (Ultraschall)."""

    def __init__(self, title, instruction, zmin, zmax, hold_s, scale_max=40, limit_s=30):
        self.title = title
        self.instruction = instruction
        self.zmin, self.zmax, self.hold_s, self.scale_max = zmin, zmax, hold_s, scale_max
        self.limit_s = limit_s          # Zeitbudget fuers Level; abgelaufen = Level gefailed
        self.reset()

    def reset(self):
        self.hold_start = None
        self.held = 0.0
        self.d = 0.0

    def update(self):
        self.d = read_distance_cm()
        in_zone = self.zmin <= self.d <= self.zmax
        if in_zone:
            if self.hold_start is None:
                self.hold_start = time.monotonic()
            self.held = time.monotonic() - self.hold_start
            return self.held >= self.hold_s
        self.hold_start = None
        self.held = 0.0
        return False

    def progress(self):
        return min(1.0, self.held / self.hold_s)

    def gauge(self):
        return {
            "value": round(self.d, 1),
            "min": self.zmin,
            "max": self.zmax,
            "scale_max": self.scale_max,
        }


# Reihenfolge der Minispiele. Hier weitere Stages anhaengen
# (z.B. spaeter eine Serial-Stage vom Arduino).
STAGES = [
    DistanceHold(
        "Annaeherung",
        "Halte die Hand 3 s ruhig in 10-15 cm vor den Sensor.",
        zmin=10, zmax=15, hold_s=3.0,
    ),
    DistanceHold(
        "Rueckzug",
        "Jetzt weiter weg: Hand 3 s in 25-35 cm halten.",
        zmin=25, zmax=35, hold_s=3.0,
    ),
]


# ---------- Spielzustand ----------
class Game:
    def __init__(self):
        self.reset()

    def reset(self):
        self.phase = "IDLE"          # IDLE, STAGE, WIRE, DEFUSED, EXPLODED
        self.idx = 0
        self.t_end = None
        self.stage_start = None      # monotonic-Start des aktuellen Levels
        self.event = ""              # one-shot: level_passed|level_failed|defused|exploded
        self.event_level = 0         # 1-basierte Level-Nummer zum Event
        self.message = "Druecke START zum Scharfschalten."
        self.wire_baseline = {}
        for s in STAGES:
            s.reset()

    @property
    def time_left(self):
        if self.t_end is None:
            return GAME_TIME
        return max(0.0, self.t_end - time.monotonic())

    def start(self):
        self.reset()
        self.phase = "STAGE"
        self.idx = 0
        self.t_end = time.monotonic() + GAME_TIME
        self.stage_start = time.monotonic()
        STAGES[0].reset()

    def to_dict(self):
        cur = STAGES[self.idx] if self.phase == "STAGE" and self.idx < len(STAGES) else None
        return {
            "phase": self.phase,
            "time_left": round(self.time_left, 1),
            "stage_index": self.idx + 1 if self.phase == "STAGE" else len(STAGES),
            "stage_total": len(STAGES),
            "title": cur.title if cur else ("Drahtbank" if self.phase == "WIRE" else ""),
            "instruction": cur.instruction if cur else "",
            "progress": round(cur.progress(), 3) if cur else 0.0,
            "gauge": cur.gauge() if cur else None,
            "message": self.message,
            "hint": WIRE_HINT if self.phase == "WIRE" else "",
            "event": self.event,
            "event_level": self.event_level,
            "wires": WIRE_COLORS,
        }


game = Game()


def step():
    wire_level = len(STAGES) + 1            # das Draht-Finale ist das letzte Level

    if game.phase in ("STAGE", "WIRE") and game.time_left <= 0:
        game.event = "exploded"
        game.event_level = wire_level if game.phase == "WIRE" else game.idx + 1
        game.phase = "EXPLODED"
        game.message = "BOOM. Zeit abgelaufen."
        return

    if game.phase == "STAGE":
        cur = STAGES[game.idx]
        elapsed = time.monotonic() - game.stage_start if game.stage_start else 0.0
        if elapsed > cur.limit_s:
            game.event = "level_failed"
            game.event_level = game.idx + 1
            game.phase = "EXPLODED"
            game.message = f"BOOM. Level {game.idx + 1} nicht rechtzeitig geschafft."
            return
        rem = max(0.0, cur.limit_s - elapsed)
        solved = cur.update()
        if isinstance(cur, DistanceHold) and cur.hold_start is not None and not solved:
            game.message = f"Halten... {cur.held:.1f}/{cur.hold_s:.0f}s  ·  noch {rem:.0f}s"
        elif not solved:
            game.message = f"Bring dich in die Zielzone.  ·  noch {rem:.0f}s"
        if solved:
            passed = game.idx + 1
            game.idx += 1
            game.event = "level_passed"
            game.event_level = passed
            if game.idx >= len(STAGES):
                game.phase = "WIRE"
                game.message = "Letzte Phase: entschaerfe die Bombe."
                game.wire_baseline = {c: wire_intact(c) for c in WIRE_COLORS}
            else:
                STAGES[game.idx].reset()
                game.stage_start = time.monotonic()
                game.message = "Stufe geschafft. Naechste Aufgabe."

    elif game.phase == "WIRE":
        for c in WIRE_COLORS:
            if game.wire_baseline.get(c, True) and not wire_intact(c):
                if c == CORRECT_WIRE:
                    game.event = "defused"
                    game.event_level = wire_level
                    game.phase = "DEFUSED"
                    game.message = "ENTSCHAERFT. Gut gemacht."
                else:
                    game.event = "exploded"
                    game.event_level = wire_level
                    game.phase = "EXPLODED"
                    game.message = f"BOOM. {c.upper()} war falsch."
                return


# ---------- WebSocket-Broadcast ----------
app = FastAPI()
clients: set[WebSocket] = set()


async def game_loop():
    while True:
        step()
        payload = game.to_dict()
        dead = []
        for ws in clients:
            try:
                await ws.send_json(payload)
            except Exception:
                dead.append(ws)
        for ws in dead:
            clients.discard(ws)
        game.event = ""          # Event ist one-shot: nach genau einem Broadcast loeschen
        game.event_level = 0
        await asyncio.sleep(TICK)


@app.on_event("startup")
async def _startup():
    asyncio.create_task(game_loop())


@app.get("/")
async def index():
    return FileResponse(os.path.join(os.path.dirname(__file__), "..", "frontend", "index.html"))


@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    await ws.accept()
    clients.add(ws)
    try:
        while True:
            msg = await ws.receive_json()
            action = msg.get("action")
            if action == "start":
                game.start()
            elif action == "reset":
                game.reset()
    except WebSocketDisconnect:
        clients.discard(ws)
    except Exception:
        clients.discard(ws)


# ---------- SIM-Endpunkte (Entwicklung ohne Hardware) ----------
if SIM:
    @app.get("/sim/dist/{cm}")
    async def sim_dist(cm: float):
        _sim["dist"] = cm
        return {"dist": cm}

    @app.get("/sim/cut/{wire}")
    async def sim_cut(wire: str):
        _sim["cut"].add(wire)
        return {"cut": list(_sim["cut"])}
