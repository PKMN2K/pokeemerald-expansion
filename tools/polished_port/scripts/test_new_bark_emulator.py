#!/usr/bin/env python3
"""Walk every original New Bark door/stair through real input in a fresh ROM."""
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile

from build_new_bark import MAPS, source_files
from build_johto_demo import ROOT, palette_rows
from test_johto_demo_emulator import Game, A, B, START, RIGHT, LEFT, UP, DOWN
from stage_johto_time_palettes import stage, load_source_palettes
from stage_johto_roof_palettes import load_roof_tables, roof_colors, apply_roof_colors

DIRECTIONS = ((RIGHT, 1, 0), (LEFT, -1, 0), (UP, 0, -1), (DOWN, 0, 1))


def map_data(name):
    data = json.loads((ROOT / f"data/maps/{name}/map.json").read_text())
    layouts = json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"]
    layout = next(row for row in layouts if row["id"] == data["layout"])
    raw = (ROOT / layout["blockdata_filepath"]).read_bytes()
    data["width"], data["height"] = layout["width"], layout["height"]
    data["cells"] = struct.unpack(f"<{len(raw) // 2}H", raw)
    return data


def walk(game, data, goal):
    """Use the actual converted collision grid, avoiding other warp tiles."""
    width, height = data["width"], data["height"]
    blocked = {(row["x"], row["y"]) for row in data["object_events"] + data["warp_events"]}
    blocked.discard(goal)
    start = game.position()
    queue = deque([start])
    paths = {start: []}
    while queue:
        pos = queue.popleft()
        if pos == goal:
            for key in paths[pos]:
                game.step(key)
            return
        for key, dx, dy in DIRECTIONS:
            target = pos[0] + dx, pos[1] + dy
            if target in paths or target in blocked or not (0 <= target[0] < width and 0 <= target[1] < height):
                continue
            cell = data["cells"][target[1] * width + target[0]]
            if cell & 0xC00 or cell >> 12 != 3:
                continue
            paths[target] = paths[pos] + [key]
            queue.append(target)
    raise AssertionError(f"No path in {data['name']} from {start} to {goal}")


def warp_behavior(data, warp):
    metatile = data["cells"][warp["y"] * data["width"] + warp["x"]] & 0x3FF
    stem = "outdoor" if data["name"] == "PolishedNewBarkTown" else "lab" if data["name"] == "PolishedElmsLab" else "house"
    bank = "secondary" if metatile >= 512 else "primary"
    raw = (ROOT / f"data/tilesets/{bank}/polished_new_bark_{stem}/metatile_attributes.bin").read_bytes()
    from build_johto_demo import behavior_ids
    return {value: key for key, value in behavior_ids().items()}[struct.unpack_from("<H", raw, metatile % 512 * 2)[0]]


def check_player_palette(game, expected):
    tags = struct.unpack("<16H", game.read(game.sym["sSpritePaletteTags"], 32))
    text = (ROOT / "include/constants/event_objects.h").read_text()
    tag = int(re.search(r"#define OBJ_EVENT_PAL_TAG_POLISHED_CHRIS\s+(0x[0-9A-Fa-f]+)", text)[1], 16)
    offset = (16 + tags.index(tag)) * 32
    for buffer in ("gPlttBufferUnfaded", "gPlttBufferFaded"):
        assert game.read(game.sym[buffer] + offset, 32) == expected, f"Player palette: {buffer}"


def run(args):
    args.output.mkdir(parents=True, exist_ok=True)
    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text())
    group_name = "gMapGroup_PolishedJohtoDemo"
    group = groups["group_order"].index(group_name)
    maps = {"Polished" + name: map_data("Polished" + name) for name in MAPS}
    ids = {name: (group, groups[group_name].index(name)) for name in maps}
    by_id = {data["id"]: name for name, data in maps.items()}
    checks, checked_warps = [], []
    palettes = stage(load_source_palettes())[0]
    roofs = load_roof_tables()
    rows = palette_rows(source_files()["gfx/overworld/npc_sprites.pal"].decode())
    obj = {period: struct.pack("<16H", *rows[index * 20], *([0] * 12))
           for index, period in enumerate(("morn", "day", "nite", "eve"))}
    # LoadTilesetPalette reserves BG color zero for Emerald's black backdrop.
    # Source white is duplicated at index four by the graphics converter.
    indoor = b"\0\0" + (ROOT / "data/tilesets/primary/polished_new_bark_house/palettes.bin").read_bytes()[2:224]
    with tempfile.TemporaryDirectory(prefix="new-bark-emulator-") as tmp:
        game = Game(args.driver.resolve(), args.rom.resolve(), args.elf.resolve(), args.output.resolve(), Path(tmp))
        try:
            game.frames(1400)
            for key in [START] + [A] * 12:
                game.press(key, 240)
                if game.u32(game.sym["gMain"] + 4) == game.sym["CB2_Overworld"] | 1:
                    break
            assert game.position() == (6, 8)
            walk(game, map_data("PolishedJohtoDemoOutdoor"), (18, 9))
            game.face(UP)
            game.press(A, 120)
            game.press(A, 240)
            assert game.map_number() == ids["PolishedNewBarkTown"], game.map_number()
            assert game.position() == (8, 10)
            checks.append("demo guide reaches original New Bark Town through the travel prompt")
            town = maps["PolishedNewBarkTown"]
            for hour, period in ((7, "morn"), (10, "day"), (19, "eve"), (23, "nite")):
                game.set_hour(hour)
                expected = apply_roof_colors(palettes[f"standard.{period}"], roof_colors(roofs, period, map_group=24)[0])
                for buffer in ("gPlttBufferUnfaded", "gPlttBufferFaded"):
                    assert game.read(game.sym[buffer], 224) == expected
                game.check_sprite_palettes(obj[period])
                game.screenshot("new-bark-" + period)
            checks.append("original town uses all four source background, roof and sprite palettes")
            game.snapshot("PolishedNewBarkTown")
            # Breadth-first traversal creates snapshots only by normal gameplay.
            # Then every directed door/stair edge is tested, including both
            # tiles of wide indoor exits and the sideways Elm house entrance.
            discovered = {"PolishedNewBarkTown"}
            queue = deque(["PolishedNewBarkTown"])
            while queue:
                origin = queue.popleft()
                data = maps[origin]
                for index, warp in enumerate(data["warp_events"]):
                    game.restore(origin)
                    goal = warp["x"], warp["y"]
                    if game.position() == goal:
                        # Leave a staircase landing before walking back onto it.
                        for _, dx, dy in DIRECTIONS:
                            candidate = goal[0] + dx, goal[1] + dy
                            if not (0 <= candidate[0] < data["width"] and 0 <= candidate[1] < data["height"]):
                                continue
                            cell = data["cells"][candidate[1] * data["width"] + candidate[0]]
                            if not cell & 0xC00 and cell >> 12 == 3:
                                walk(game, data, candidate)
                                break
                        assert game.position() != goal
                    walk(game, data, goal)
                    behavior = warp_behavior(data, warp)
                    if behavior.endswith("ARROW_WARP"):
                        key = {"MB_SOUTH_ARROW_WARP": DOWN, "MB_WEST_ARROW_WARP": LEFT, "MB_EAST_ARROW_WARP": RIGHT}[behavior]
                        # Allow the player's turn animation to finish before
                        # attempting a sideways arrow warp in a new direction.
                        game.frames(20, key)
                        game.frames(240)
                    else:
                        game.frames(240)
                    destination = by_id[warp["dest_map"]]
                    assert game.map_number() == ids[destination], (origin, index, destination, game.map_number(), game.position())
                    target = maps[destination]["warp_events"][int(warp["dest_warp_id"])]
                    landing = target["x"], target["y"]
                    if warp_behavior(maps[destination], target) == "MB_NON_ANIMATED_DOOR":
                        landing = landing[0], landing[1] + 1
                    assert game.position() == landing, (origin, index, destination, game.position(), landing)
                    expected = apply_roof_colors(palettes["standard.nite"], roof_colors(roofs, "nite", map_group=24)[0]) if destination == "PolishedNewBarkTown" else indoor
                    for buffer in ("gPlttBufferUnfaded", "gPlttBufferFaded"):
                        actual = game.read(game.sym[buffer], 224)
                        differences = [(i, a, b) for i, (a, b) in enumerate(zip(struct.unpack("<112H", actual), struct.unpack("<112H", expected))) if a != b]
                        assert not differences, (destination, buffer, differences[:12])
                    check_player_palette(game, obj["nite" if destination == "PolishedNewBarkTown" else "day"])
                    checked_warps.append({"map": origin, "warp": index, "destination": destination, "landing": list(landing)})
                    print("PASS: warp", origin, index, "->", destination, landing, flush=True)
                    if destination not in discovered:
                        discovered.add(destination)
                        queue.append(destination)
                        game.snapshot(destination)
                        game.screenshot(destination)
            assert len(discovered) == 8 and len(checked_warps) == 19
            checks.append("all 19 source warp events reach the correct door/stair and can be left again")
            checks.append("all seven interiors use fixed source day colors and restore the outdoor night palette")
            for name, position, key, shot in (
                ("PolishedNewBarkTown", (8, 9), UP, "town-sign"),
                ("PolishedPlayersHouse1F", (1, 2), UP, "home-fridge"),
                ("PolishedLyrasHouse2F", (4, 2), UP, "lyra-pc"),
            ):
                game.restore(name)
                walk(game, maps[name], position)
                game.face(key)
                game.interact(shot)
                assert game.read(game.sym["sGlobalScriptContext"] + 1, 1) == b"\0"
            checks.append("source town sign, home furniture and Lyra's PC dialogue release control")
            game.restore("PolishedElmsLab")
            walk(game, maps["PolishedElmsLab"], (2, 2))
            game.face(UP)
            game.press(A, 120)
            game.press(A, 120)
            game.press(B, 60)
            assert game.read(game.sym["sGlobalScriptContext"] + 1, 1) == b"\0"
            checks.append("Elm's healing machine accepts the native healing interaction")
            game.restore("PolishedNewBarkTown")
            walk(game, town, (6, 9))
            game.face(UP)
            game.press(A, 120)
            for _ in range(6):
                game.press(A, 80)
                if game.map_number() == (group, 0):
                    game.frames(200)
                    break
            assert game.map_number() == (group, 0) and game.position() == (6, 8)
            checks.append("teacher's return prompt restores the original demo garden")
        except Exception:
            game.screenshot("failure")
            raise
        finally:
            game.close()
    report = {"rom_sha256": hashlib.sha256(args.rom.read_bytes()).hexdigest(), "checks": checks, "warps": checked_warps}
    (args.output / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    for check in checks:
        print("PASS:", check)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    run(parser.parse_args())
