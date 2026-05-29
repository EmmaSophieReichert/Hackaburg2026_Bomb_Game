#!/usr/bin/env python3
"""Single source of truth fuers Frontend.

Quellen sind frontend/index.html und frontend/app.js. Dieses Skript bettet
beide als PROGMEM-Literale in Header der ESP-Sketches ein, damit alle
Build-Ziele dasselbe Frontend ausliefern. Nach jeder Frontend-Aenderung
ausfuehren:

    python3 sync_frontend.py            # schreibt die Header neu
    python3 sync_frontend.py --check    # nur pruefen (Exit 1 wenn out of sync)
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SKETCH_DIRS = [ROOT / "bomb-esp32", ROOT / "bomb-esp8266"]

# je Asset: Quelle, Header-Dateiname, C-Variablenname, Raw-String-Delimiter
ASSETS = [
    ("frontend/index.html", "index_html.h", "INDEX_HTML", "rawhtml"),
    ("frontend/app.js",     "app_js.h",     "APP_JS",     "rawjs"),
]

HEADER = """\
#pragma once
#include <pgmspace.h>

// AUTOGENERIERT von sync_frontend.py aus {src} — NICHT direkt editieren.
// Frontend-Aenderungen in {src} machen und das Skript erneut laufen lassen.
"""


def render(src_name: str, var: str, delim: str, content: str) -> str:
    if f"){delim}\"" in content:
        sys.exit(f"FEHLER: {src_name} enthaelt die Delimiter-Sequenz '){delim}\"' — "
                 f"Delimiter in sync_frontend.py aendern.")
    head = HEADER.format(src=src_name)
    return f'{head}const char {var}[] PROGMEM = R"{delim}({content}){delim}";\n'


def main() -> int:
    check = "--check" in sys.argv
    stale = []

    for src_rel, out_name, var, delim in ASSETS:
        src = ROOT / src_rel
        if not src.exists():
            sys.exit(f"FEHLER: Quelle fehlt: {src}")
        out = render(src_rel, var, delim, src.read_text(encoding="utf-8"))
        for d in SKETCH_DIRS:
            tgt = d / out_name
            current = tgt.read_text(encoding="utf-8") if tgt.exists() else None
            if current == out:
                print(f"ok    {tgt.relative_to(ROOT)}")
                continue
            stale.append(tgt)
            if check:
                print(f"STALE {tgt.relative_to(ROOT)}")
            else:
                tgt.write_text(out, encoding="utf-8")
                print(f"wrote {tgt.relative_to(ROOT)}")

    if check and stale:
        print("\nout of sync — 'python3 sync_frontend.py' ausfuehren.")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
