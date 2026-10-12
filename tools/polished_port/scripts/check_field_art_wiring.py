#!/usr/bin/env python3
"""Static regression gate for the pre-Prism Polished Crystal field-art port.

Run at the repository root:
    python3 tools/polished_port/scripts/check_field_art_wiring.py

This is not a ROM-build or in-game animation test.
"""
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
ASSETS = {
    "walking": ("graphics/object_events/pics/people/polished_chris/walking.png", (32, 32)),
    "running": ("graphics/object_events/pics/people/polished_chris/running.png", (32, 32)),
    "bike": ("graphics/object_events/pics/people/polished_chris/bike.png", (32, 32)),
    "surfing": ("graphics/object_events/pics/people/polished_chris/surfing.png", (32, 32)),
    "fishing": ("graphics/object_events/pics/people/polished_chris/fishing.png", (32, 32)),
    "surf_fishing": ("graphics/object_events/pics/people/polished_chris/surf_fishing.png", (32, 32)),
    "field_move": ("graphics/object_events/pics/people/polished_chris/field_move.png", (32, 32)),
}
OBJECT_NAMES = (
    "POLISHED_CHRIS_NORMAL", "POLISHED_CHRIS_BIKE",
    "POLISHED_CHRIS_SURFING", "POLISHED_CHRIS_FISHING",
    "POLISHED_CHRIS_SURF_FISHING", "POLISHED_CHRIS_FIELD_MOVE",
)
errors = []


def check(condition, message):
    if not condition:
        errors.append(message)


def read(path):
    p = ROOT / path
    if not p.is_file():
        errors.append("missing: " + path)
        return ""
    return p.read_text(encoding="utf-8")


graphics = read("src/data/object_events/object_event_graphics.h")
pics = read("src/data/object_events/object_event_pic_tables.h")
infos = read("src/data/object_events/object_event_graphics_info.h")
pointers = read("src/data/object_events/object_event_graphics_info_pointers.h")
constants = read("include/constants/event_objects.h")
player = read("src/field_player_avatar.c")

for name, (path, (frame_w, frame_h)) in ASSETS.items():
    p = ROOT / path
    if not p.is_file():
        errors.append("missing: " + path)
        continue
    with p.open("rb") as f:
        header = f.read(24)
    if len(header) < 24 or header[:8] != b"\\x89PNG\\r\\n\\x1a\\n" or header[12:16] != b"IHDR":
        errors.append("invalid PNG header: " + path)
        continue
    w, h = struct.unpack(">II", header[16:24])
    check(w % frame_w == 0 and h % frame_h == 0,
          f"{name}: {w}x{h} not aligned to {frame_w}x{frame_h} frames")
    check(path in graphics, f"{name}: graphic asset not referenced by object-event graphics header")

for name in OBJECT_NAMES:
    check("OBJ_EVENT_GFX_" + name in constants, "missing object ID " + name)
    check("gObjectEventGraphicsInfo_PolishedChris" in infos, "missing Polished Chris graphics info")
    check("OBJ_EVENT_GFX_" + name in pointers, "missing graphics pointer " + name)

check("sPicTable_PolishedChrisSurfFishing" in pics,
      "missing dedicated surf-fishing picture table")
check("POLISHED_CHRIS_SURF_FISHING" in player,
      "player fishing logic does not reference surf-fishing state")
check("POLISHED_CHRIS_FISHING" in player,
      "player fishing logic does not reference land-fishing state")

if errors:
    for error in errors:
        print("FAIL:", error, file=sys.stderr)
    sys.exit(1)
print("PASS: Polished Crystal player-field asset presence, frame alignment and references")
