#!/usr/bin/env python3
"""Stage the authentic Polished Crystal Johto 8x8 animated tile frames.

Inputs are nine pinned, unmodified 2-bit grayscale PNG blobs from
Rangi42/polishedcrystal at 3fa43192379df5c3e7b09a08e4d5d79af4f02f42.
RGBGFX stores tile images in left-to-right, top-to-bottom 8x8 order.

The original 22-step tileset animation command loop updates its 3-bit
wTileAnimationTimer at dispatch step 11, once per loop (not every call).
This script preserves source selectors and exports a staging trace. It
does not install an Emerald animation callback or alter Prism/Hoenn maps.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import re

from stage_johto_tile_graphics import (
    gba_tile_id, load_source_metatile_tables, parse_grayscale2_png,
    png_to_tiles, priority_overlay_tile_ids,
)

PIN = "3fa43192379df5c3e7b09a08e4d5d79af4f02f42"
ROOT = Path(__file__).resolve().parents[1]
PNG_FIXTURE = ROOT / "fixtures/johto_outdoor_animation_pngs.json"
TIMING_FIXTURE = ROOT / "fixtures/johto_outdoor_animation_timing.asm"
SCHEDULE_FIXTURE = ROOT / "fixtures/johto_outdoor_animation_schedule.asm"

# Fixed by original PNG layout, dimensions, and engine/tilesets/tileset_anims.asm.
# Original GBC VRAM 'vTiles2' is graphics bank1 in the migration.
SOURCE_IMAGES = (
    ("water", "water/johto_water.png", 16, 16, 4),
    ("rain_puddle", "rain/rain_puddle.png", 32, 16, 8),
    ("rain_water", "rain/rain_water.png", 32, 16, 8),
    ("flower_1", "flower/1.png", 8, 8, 1),
    ("flower_2", "flower/2.png", 8, 8, 1),
    ("whirlpool_1", "whirlpool/1.png", 8, 32, 4),
    ("whirlpool_2", "whirlpool/2.png", 8, 32, 4),
    ("whirlpool_3", "whirlpool/3.png", 8, 32, 4),
    ("whirlpool_4", "whirlpool/4.png", 8, 32, 4),
)

# Name -> (original VRAM1 tile ID, number of frames, timer bit shift).
# Selectors always wrap a power-of-two number of source frames.
TARGETS = {
    "water": (0x14, 4, 1),
    "rain_puddle": (0x1C, 8, 0),
    "rain_water": (0x1D, 8, 0),
    "flower": (0x03, 2, 1),
    "whirlpool_1": (0x31, 4, 0),
    "whirlpool_2": (0x32, 4, 0),
    "whirlpool_3": (0x41, 4, 0),
    "whirlpool_4": (0x42, 4, 0),
}
ANIMATION_DISPATCH = {
    0: "water", 1: "rain_puddle", 2: "rain_water", 4: "flower",
    5: "whirlpool_1", 6: "whirlpool_2",
    7: "whirlpool_3", 8: "whirlpool_4",
    9: "rain_puddle", 10: "rain_water",
}
ACTIONS = (
    "AnimateWaterTile", "AnimateRainPuddleTile", "AnimateRainWaterTile",
    "DoNothing", "AnimateFlowerTile", "AnimateWhirlpoolTile",
    "AnimateWhirlpoolTile", "AnimateWhirlpoolTile", "AnimateWhirlpoolTile",
    "AnimateRainPuddleTile", "AnimateRainWaterTile", "StandingTileFrame8",
    "WriteTileToBuffer", "DoNothing", "ScrollTileDown", "DoNothing",
    "ScrollTileDown", "DoNothing", "ScrollTileDown", "DoNothing",
    "ReadTileFromBuffer", "DoneTileAnimation",
)
# Pinned upstream blob SHA-1 values, *not* of a converted or redrawn image.
PINNED_PNG_BLOBS = {
    "water": "29fd57e1a3a68a139c5bfac544770998dfad845a",
    "rain_puddle": "302438f15729fa957b6b9984dd105d803d7162b4",
    "rain_water": "4b99512c5d61dd2c4524ee8499b2e42e75946528",
    "flower_1": "e3fd2867bc16b3bfe5ce8880692f6b6c328521be",
    "flower_2": "901e34a1fbe72e92fdbb2990171d14bd5284e863",
    "whirlpool_1": "4641dbc0d0e2bedb4100f5c34e29cf298b94ddf0",
    "whirlpool_2": "ce542405364971ab2c9e5b187bf2f36a3bc55cc1",
    "whirlpool_3": "55b629440e659d0e657c2308bac6dd91f22fc05d",
    "whirlpool_4": "2175c5957ea414c68440fd8a80cd4b48e58c02a4",
}


def load_images(fixture=PNG_FIXTURE):
    doc = json.loads(fixture.read_text(encoding="utf-8"))
    if doc.get("source_repository") != "Rangi42/polishedcrystal" or doc.get("source_commit") != PIN:
        raise ValueError("animation source revision changed")
    assets = doc.get("assets")
    if not isinstance(assets, dict) or set(assets) != set(PINNED_PNG_BLOBS):
        raise ValueError("animation PNG source asset inventory changed")
    decoded = {}
    for name, original_path, width, height, frames in SOURCE_IMAGES:
        item = assets[name]
        if (item.get("source_path") != "gfx/tilesets/" + original_path
            or item.get("git_blob_sha") != PINNED_PNG_BLOBS[name]
            or (item.get("width"), item.get("height"), item.get("frame_count"))
                != (width, height, frames)):
            raise ValueError(f"{name}: source PNG identity/dimensions mismatch")
        raw = base64.b64decode(item["png_base64"], validate=True)
        sha = hashlib.sha1(b"blob " + str(len(raw)).encode("ascii") + b"\x00" + raw).hexdigest()
        if sha != PINNED_PNG_BLOBS[name]:
            raise ValueError(f"{name}: unpinned or edited PNG asset")
        rows = parse_grayscale2_png(raw, expected_width=width, expected_height=height)
        tiles = png_to_tiles(rows)
        if len(tiles) != frames:
            raise ValueError(f"{name}: source frame count mismatch")
        decoded[name] = tiles
    return decoded


def original_dispatch():
    source = SCHEDULE_FIXTURE.read_text(encoding="utf-8")
    entries = []
    for line in source.splitlines():
        m = re.fullmatch(r"\s*dw\s+(.+?),\s*(\w+)\s*", line)
        if m:
            entries.append((m.group(1).strip(), m.group(2)))
    if len(entries) != 22 or tuple(action for _, action in entries) != ACTIONS:
        raise ValueError("the 22-step original animation dispatch schedule changed")
    # Destination tile numbers, whirlpool frame pointers and the scrolling
    # buffer target must match original source, not a guessed animation path.
    expected = {
        0: "$14", 1: "$1c", 2: "$1d", 5: "WhirlpoolFrames1",
        6: "WhirlpoolFrames2", 7: "WhirlpoolFrames3",
        8: "WhirlpoolFrames4", 9: "$1c", 10: "$1d",
        12: "$35", 20: "$35",
    }
    for step, token in expected.items():
        if token not in entries[step][0]:
            raise ValueError(f"dispatch step {step} source pointer changed")
    return entries


def verify_source_timing():
    asm = TIMING_FIXTURE.read_text(encoding="utf-8")
    checks = (
        (r"StandingTileFrame8:\s*ld a, \[wTileAnimationTimer\]\s*"
         r"inc a\s*and %111\s*ld \[wTileAnimationTimer\], a",
         "timer once per 22-step cycle, modulo eight"),
        (r"AnimateWaterTile:.*?maskbits 4, 1",
         "water four frames, 2-timer phase hold"),
        (r"AnimateRainPuddleTile:.*?maskbits 8",
         "rain puddle eight timer phases"),
        (r"AnimateRainWaterTile:.*?maskbits 8",
         "rain water eight timer phases"),
        (r"AnimateFlowerTile:.*?maskbits 2, 1",
         "flower two frames, 2-timer phase hold"),
        (r"AnimateWhirlpoolTile:.*?maskbits 4",
         "four whirlpool frames"),
        (r"WhirlpoolFrames1:\s*dw vTiles2 tile \$31, WhirlpoolTiles1",
         "whirlpool destination 1"),
        (r"WhirlpoolFrames4:\s*dw vTiles2 tile \$42, WhirlpoolTiles4",
         "whirlpool destination 4"),
    )
    for pattern, meaning in checks:
        if not re.search(pattern, asm, flags=re.S):
            raise ValueError(f"original animation timing changed: {meaning}")
    return True


def build_frames():
    original_dispatch()
    verify_source_timing()
    raw = load_images()
    groups = {
        name: raw[name] for name in TARGETS if name != "flower"
    }
    groups["flower"] = raw["flower_1"] + raw["flower_2"]
    if sum(len(tiles) for tiles in groups.values()) != 38:
        raise ValueError("expected 38 authentic source animation frames")
    source_meta, source_attrs = load_source_metatile_tables()
    priority_source_ids = set(priority_overlay_tile_ids(source_meta, source_attrs))
    for name, (gbc_tile, count, shift) in TARGETS.items():
        if len(groups[name]) != count or count & (count - 1) or shift not in (0, 1):
            raise ValueError(f"invalid authentic frame selector for {name}")
        gba_id = gba_tile_id(gbc_tile, 1)
        if gba_id in priority_source_ids:
            raise ValueError(f"animated GBA tile {gba_id} needs a matching BG1 mask update")
        for tile in groups[name]:
            if len(tile) != 32 or any(
                (value & 15) not in (1, 2, 3, 4) or
                (value >> 4) not in (1, 2, 3, 4)
                for value in tile
            ):
                raise ValueError(f"{name}: non-authentic/transparent source pixel")
    return groups


def frame_index(name, timer):
    if name not in TARGETS or not isinstance(timer, int) or not 0 <= timer <= 7:
        raise ValueError("invalid original source animation name or 3-bit timer")
    _, count, bit_shift = TARGETS[name]
    return (timer >> bit_shift) & (count - 1)


def trace_source_cycles(cycles=8):
    if not isinstance(cycles, int) or not 1 <= cycles <= 16:
        raise ValueError("source trace needs 1..16 complete 22-step dispatch cycles")
    entries = original_dispatch()
    verify_source_timing()
    timer = 0
    trace = []
    for cycle in range(cycles):
        for step, (_, action) in enumerate(entries):
            before = timer
            item = {
                "source_cycle": cycle,
                "dispatch_step": step,
                "source_action": action,
                "timer_before_step": before,
            }
            if step in ANIMATION_DISPATCH:
                stream = ANIMATION_DISPATCH[step]
                original_id, frame_count, shift = TARGETS[stream]
                item.update({
                    "animation": stream,
                    "source_vram_bank": 1,
                    "source_vram_tile": original_id,
                    "gba_primary_tile_id": gba_tile_id(original_id, 1),
                    "frame_index": frame_index(stream, timer),
                    "source_frame_count": frame_count,
                    "selector_timer_shift": shift,
                })
            if action == "StandingTileFrame8":
                timer = (timer + 1) & 7
            elif action in ("WriteTileToBuffer", "ReadTileFromBuffer"):
                # Original 2bpp VRAM writes/3-step downscroll still require
                # authentic GBA byte-level emulation, NOT arbitrary 4bpp
                # scroll frames. The animated scroll tile is GBC bank1 $35.
                item["pending_scroll_buffer_gba_tile"] = gba_tile_id(0x35, 1)
                item["runtime_scroll_emulated"] = False
            elif action == "ScrollTileDown":
                item["runtime_scroll_emulated"] = False
            item["timer_after_step"] = timer
            trace.append(item)
    return trace


def stage(output):
    groups = build_frames()
    output.mkdir(parents=True, exist_ok=True)
    report = {
        "status": "authentic_4bpp_frames_staged_not_live",
        "source_repository": "Rangi42/polishedcrystal",
        "source_revision": PIN,
        "pinned_png_files": len(SOURCE_IMAGES),
        "source_total_animation_frames": 38,
        "source_dispatch_steps_per_cycle": 22,
        "timer_increment_source_dispatch_step": 11,
        "timer_wrap_modulus": 8,
        "one_source_png_tile_per_animation_frame": True,
        "source_cgb_palette_color0_stays_opaque_at_gba_color_index4": True,
        "overlaps_source_priority_mask_tiles": False,
        "animations": {},
        "scroll_buffer_tile": {"source_gbc_vram1": 0x35,
                               "gba_primary_tile_id": gba_tile_id(0x35, 1),
                               "three_original_scroll_down_commands": 3,
                               "runtime_4bpp_scroll_verified": False},
        "all_source_tileset_animations_installed": False,
        "emerald_callback_registered": False,
        "johto_map_registered": False,
        "ready_for_live_registration": False,
    }
    for name, frames in groups.items():
        original_tile, expected_count, bit_shift = TARGETS[name]
        payload = b"".join(frames)
        filename = f"johto.{name}.staging.4bpp"
        (output / filename).write_bytes(payload)
        report["animations"][name] = {
            "source_animation_frames": expected_count,
            "gba_4bpp_frame_bytes": 32,
            "gba_4bpp_staged_bytes": len(payload),
            "source_vram_bank": 1,
            "source_vram_tile": original_tile,
            "gba_primary_tile_id": gba_tile_id(original_tile, 1),
            "frame_select": f"(original_timer >> {bit_shift}) & {expected_count - 1}",
            "source_frame_index_over_8_timer_phases": [
                frame_index(name, timer) for timer in range(8)
            ],
            "filename": filename,
        }
    (output / "johto.animations.staging.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    (output / "johto.animations.dispatch.staging.json").write_text(
        json.dumps(trace_source_cycles(8), indent=2) + "\n",
        encoding="utf-8"
    )
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    report = stage(args.output)
    print(f"Staged {report['source_total_animation_frames']} authentic GBA 4bpp "
          f"frames across {len(report['animations'])} animated 8x8 targets.")
    print("Staged 8 original 22-step dispatch cycles and 3-bit timer selections.")
    print("No live callback: 2bpp tile $35 scrolling and object priority need validation.")


if __name__ == "__main__":
    main()
