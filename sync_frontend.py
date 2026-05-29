#!/usr/bin/env python3
"""Single source of truth fuers Frontend.

Quelle ist frontend/index.html. Dieses Skript bettet sie als PROGMEM-Literal
in die index_html.h der ESP-Sketches ein, damit alle Build-Ziele dasselbe
Frontend ausliefern. Nach jeder Aenderung an frontend/index.html ausfuehren:

    python3 sync_frontend.py            # schreibt die Header neu
    python3 sync_frontend.py --check    # nur pruefen (Exit 1 wenn out of sync)
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "frontend" / "index.html"
TARGETS = [
    ROOT / "bomb-esp32" / "index_html.h",
    ROOT / "bomb-esp8266" / "index_html.h",
]
DELIM = "rawhtml"

HEADER = """\
#pragma once
#include <pgmspace.h>

// AUTOGENERIERT von sync_frontend.py aus frontend/index.html — NICHT direkt
// editieren. Frontend-Aenderungen in frontend/index.html machen und das Skript
// erneut laufen lassen.
"""


def render(html: str) -> str:
    if f"){DELIM}\"" in html:
        sys.exit(f"FEHLER: Quelle enthaelt die Delimiter-Sequenz '){DELIM}\"' — "
                 f"Delimiter in sync_frontend.py aendern.")
    return f'{HEADER}const char INDEX_HTML[] PROGMEM = R"{DELIM}({html}){DELIM}";\n'


def main() -> int:
    check = "--check" in sys.argv
    if not SRC.exists():
        sys.exit(f"FEHLER: Quelle fehlt: {SRC}")
    out = render(SRC.read_text(encoding="utf-8"))

    stale = []
    for tgt in TARGETS:
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
