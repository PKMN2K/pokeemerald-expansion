#!/usr/bin/env python3
"""Exercise New Bark's warps, residents and schedules through real GBA input."""
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
    blocked = {(row["x"], row["y"]) for row in data["warp_events"]}
    # Reserve each resident's whole walking range, not just its spawn tile.
    for row in data["object_events"]:
        for dx in range(-row["movement_range_x"], row["movement_range_x"] + 1):
            for dy in range(-row["movement_range_y"], row["movement_range_y"] + 1):
                blocked.add((row["x"] + dx, row["y"] + dy))
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


def actors(game):
    group, number = game.map_number()
    count = int(re.search(r"#define OBJECT_EVENTS_COUNT\s+(\d+)",
        (ROOT / "include/constants/global.h").read_text())[1])
    raw = game.read(game.sym["gObjectEvents"], count * 0x24)
    result = {}
    for start in range(0, len(raw), 0x24):
        actor = raw[start:start + 0x24]
        if actor[0] & 1 and actor[9:11] == bytes((number, group)) and not actor[2] & 1:
            result[actor[8]] = {"position": tuple(v - 7 for v in struct.unpack_from("<hh", actor, 0x10)),
                "graphics": struct.unpack_from("<H", actor, 4)[0], "direction": actor[0x18] & 15,
                "invisible": bool(actor[1] & 0x20)}
    return result


def resident_id(data, name):
    return next(i + 1 for i, obj in enumerate(data["object_events"]) if obj["local_id"] == name)


def travel(game, data, destination):
    warp = next(row for row in data["warp_events"] if row["dest_map"] == destination["id"])
    walk(game, data, (warp["x"], warp["y"]))
    behavior = warp_behavior(data, warp)
    if behavior.endswith("ARROW_WARP"):
        game.frames(20, {"MB_SOUTH_ARROW_WARP": DOWN, "MB_WEST_ARROW_WARP": LEFT, "MB_EAST_ARROW_WARP": RIGHT}[behavior])
    game.frames(240)


def check_resident_palette(game, data, local_id, color, period, rows):
    actor = actors(game)[local_id]
    assert not actor["invisible"]
    info = game.u32(game.sym["gObjectEventGraphicsInfoPointers"] + actor["graphics"] * 4)
    tag = struct.unpack("<H", game.read(info + 2, 2))[0]
    tags = struct.unpack("<16H", game.read(game.sym["sSpritePaletteTags"], 32))
    assert tag in tags
    offset = (16 + tags.index(tag)) * 32
    source_period = ("morn", "day", "nite", "eve").index(period)
    expected = struct.pack("<16H", *rows[source_period * 20 + color], *([0] * 12))
    for buffer in ("gPlttBufferUnfaded", "gPlttBufferFaded"):
        assert game.read(game.sym[buffer] + offset, 32) == expected, (data["name"], local_id, period, buffer)


def talk_to_resident(game, data, local_id, shot, leave_open=False):
    """Approach the resident's actual position, including moving actors."""
    for attempt in range(30):
        npc = actors(game).get(local_id)
        assert npc is not None, (data["name"], local_id)
        x, y = npc["position"]
        for key, dx, dy in DIRECTIONS:
            approach = x - dx, y - dy
            if not (0 <= approach[0] < data["width"] and 0 <= approach[1] < data["height"]):
                continue
            try:
                walk(game, data, approach)
            except AssertionError:
                continue
            game.face(key)
            if actors(game)[local_id]["position"] != (x, y):
                break
            game.press(A, 240)
            if game.read(game.sym["sGlobalScriptContext"] + 1, 1) != b"\0":
                assert struct.unpack("<H", game.read(game.sym["gSpecialVar_LastTalked"], 2))[0] == local_id
                game.screenshot(shot)
                if not leave_open:
                    close_resident_dialogue(game)
                return
        game.frames(30)
    raise AssertionError(f"Cannot talk to {data['name']} resident {local_id}")


def close_resident_dialogue(game):
    for _ in range(32):
        game.press(A, 45)
        if game.read(game.sym["sGlobalScriptContext"] + 1, 1) == b"\0":
            return
    raise AssertionError("Resident dialogue did not release control")


def save_game(game):
    save_counter = game.u32(game.sym["gSaveCounter"])
    game.press(START, 40)
    count = game.read(game.sym["sNumStartMenuActions"], 1)[0]
    actions = game.read(game.sym["sCurrentStartMenuActions"], count)
    # Follow the native menu's Save callback, rather than assuming a fixed row.
    save_action = next(i for i in actions if game.u32(game.sym["sStartMenuItems"] + i * 8 + 4)
        == game.sym["StartMenuSaveCallback"] | 1)
    for _ in range(count):
        cursor = game.read(game.sym["sStartMenuCursorPos"], 1)[0]
        if actions[cursor] == save_action:
            break
        game.press(DOWN, 12)
    else:
        raise AssertionError("Save menu entry not found")
    game.press(A, 100)
    # Follow live save callbacks, explicitly choose YES on either confirmation
    # prompt, and require an actual successful save rather than a fixed wait.
    for _ in range(180):
        callback = game.u32(game.sym["sSaveDialogCallback"]) & ~1
        if callback in (game.sym["SaveConfirmInputCallback"], game.sym["SaveOverwriteInputCallback"]):
            game.press(UP, 12)
            game.press(A, 30)
        elif callback == game.sym["SaveReturnSuccessCallback"]:
            game.press(A, 120)
            break
        elif callback in (game.sym["SaveErrorCallback"], game.sym["SaveReturnErrorCallback"]):
            raise AssertionError("Native save reported an error")
        else:
            game.frames(20)
    else:
        raise AssertionError("Native save did not report success")
    assert game.u32(game.sym["gSaveCounter"]) != save_counter, "Native save counter did not advance"
    game.press(B, 40)
    game.screenshot("resident-save")


def continue_game(game):
    game.command("reset")
    game.frames(1400)
    for key in [START] + [A] * 12:
        game.press(key, 240)
        if game.u32(game.sym["gMain"] + 4) == game.sym["CB2_Overworld"] | 1:
            return
    raise AssertionError("Continue did not reach the overworld")


def check_residents(game, maps, rows, checks):
    town, neighbors, elms = (maps["Polished" + name] for name in ("NewBarkTown", "PlayersNeighborsHouse", "ElmsHouse"))
    boy = resident_id(town, "LOCALID_NEW_BARK_SCHOOLBOY")
    husband = resident_id(neighbors, "LOCALID_NEIGHBORS_HUSBAND")
    game.restore("PolishedNewBarkTown")
    # Both sides of every source time boundary, including midnight. Native
    # Expansion TOD settings must not change Crystal's 05:00/17:00 schedule.
    for hour in (4, 5, 8, 9, 16, 17, 20, 21, 0):
        game.set_hour(hour)
        travel(game, town, neighbors)
        assert (husband in actors(game)) == (hour < 5 or hour >= 17), ("indoors", hour, actors(game))
        if hour == 17:
            game.snapshot("residents-evening-indoors")
        travel(game, neighbors, town)
        walk(game, town, (7, 14))
        assert (boy in actors(game)) == (5 <= hour < 17), ("outdoors", hour, actors(game))
        if hour == 16:
            game.snapshot("residents-daytime-town")
        print("PASS: resident schedules at", hour, flush=True)
    checks.append("both residents obey source map-entry schedules at 04/05, 08/09, 16/17, 20/21 and midnight")

    # All six conversations; indoor sprites keep source DAY colors at night.
    cases = (
        (town, "residents-daytime-town", "LOCALID_NEW_BARK_FAT_GUY", 2, "PolishedFatGuy"),
        (town, "residents-daytime-town", "LOCALID_NEW_BARK_SCHOOLBOY", 1, "PolishedSchoolboy"),
        (neighbors, "residents-evening-indoors", "LOCALID_NEIGHBORS_DAUGHTER", 0, "PolishedBattleGirl"),
        (neighbors, "residents-evening-indoors", "LOCALID_NEIGHBORS_HUSBAND", 1, "PolishedSchoolboy"),
        (elms, "PolishedElmsHouse", "LOCALID_ELMS_WIFE", 2, "PolishedPokefanGreen"),
        (elms, "PolishedElmsHouse", "LOCALID_ELMS_SON", 3, "PolishedChild"),
    )
    for data, snapshot, name, color, graphics in cases:
        game.restore(snapshot)
        game.set_hour(10 if data == town else 23)
        local_id = resident_id(data, name)
        obj = data["object_events"][local_id - 1]
        actor = actors(game)[local_id]
        assert game.u32(game.sym["gObjectEventGraphicsInfoPointers"] + actor["graphics"] * 4) == game.sym["gObjectEventGraphicsInfo_" + graphics]
        positions = set()
        for _ in range(80):
            current = actors(game)[local_id]["position"]
            assert abs(current[0] - obj["x"]) <= obj["movement_range_x"], (name, current)
            assert abs(current[1] - obj["y"]) <= obj["movement_range_y"], (name, current)
            positions.add(current)
            game.frames(10)
        if obj["movement_range_x"] or obj["movement_range_y"]:
            assert len(positions) > 1, name
        else:
            assert len(positions) == 1, name
        check_resident_palette(game, data, local_id, color, "day", rows)
        talk_to_resident(game, data, local_id, name.lower())
        if data == town:
            for hour, period in ((7, "morn"), (19, "eve"), (23, "nite")):
                game.set_hour(hour)
                check_resident_palette(game, data, local_id, color, period, rows)
        print("PASS: resident artwork, movement, dialogue and palette", name, flush=True)
    checks.append("all six authentic resident graphics, source colors, movement bounds and conversations work in-game")

    # A time change must not remove the speaker or reschedule on a menu return.
    game.restore("residents-daytime-town")
    game.set_hour(16)
    talk_to_resident(game, town, boy, "schoolboy-before-evening", leave_open=True)
    game.set_hour(17)
    assert boy in actors(game) and game.read(game.sym["sGlobalScriptContext"] + 1, 1) != b"\0"
    close_resident_dialogue(game)
    game.press(START, 40)
    game.press(B, 40)
    assert boy in actors(game)
    travel(game, town, neighbors)
    travel(game, neighbors, town)
    walk(game, town, (7, 14))
    assert boy not in actors(game)
    checks.append("clock changes preserve the current speaker until a map reload")

    # Save a visible resident, then a hidden one; reset/Continue at the opposite
    # time so saved actors cannot override the newly calculated visibility.
    travel(game, town, neighbors)
    assert husband in actors(game)
    saved_map, saved_position = game.map_number(), game.position()
    save_game(game)
    game.set_hour(10)
    continue_game(game)
    assert game.map_number() == saved_map and game.position() == saved_position, (saved_map, saved_position, game.map_number(), game.position())
    assert husband not in actors(game), "saved evening resident survived a daytime Continue"
    save_game(game)
    game.set_hour(23)
    continue_game(game)
    assert game.map_number() == saved_map and game.position() == saved_position
    assert husband in actors(game), "saved hidden resident did not return on a nighttime Continue"
    checks.append("native Save/reset/Continue reconciles resident visibility in both directions")


def player_state(game):
    index = game.read(game.sym["gPlayerAvatar"] + 5, 1)[0]
    raw = game.read(game.sym["gObjectEvents"] + index * 36, 36)
    return {"position": list(game.position()), "direction": raw[0x18] & 15,
        "facing_locked": bool(raw[1] & 2), "frozen": bool(raw[1] & 1),
        "jump_height": struct.unpack("<h", game.read(game.sym["gSprites"] + raw[0x23] * 68 + 38, 2))[0]}


def rival_encounter(game, town, local_id, name):
    prefix = "PolishedNewBarkTown_Rival"
    text1 = game.read(game.sym[prefix + "_Text1"], 256).split(b"\xff", 1)[0] + b"\xff"
    text2 = game.read(game.sym[prefix + "_Text2"], 256).split(b"\xff", 1)[0] + b"\xff"
    assert game.position() == (2, 2) and actors(game)[local_id]["position"] == (3, 2)
    assert actors(game)[local_id]["direction"] == 4  # Still watching the window.
    game.face(RIGHT)
    game.press(A, 240)
    assert game.read(game.sym["gStringVar4"], len(text1)) == text1
    assert actors(game)[local_id]["direction"] == 4
    game.press(A, 240)
    game.screenshot(name + "-watching")
    for _ in range(16):
        if game.read(game.sym["gStringVar4"], len(text2)) == text2:
            break
        game.press(A, 80)
    else:
        raise AssertionError("Rival never delivered the second source text")
    game.frames(240)
    assert game.position() == (2, 2) and actors(game)[local_id]["direction"] == 3
    game.screenshot(name + "-response")
    sound = int(re.search(r"#define SE_M_COMET_PUNCH\s+(\d+)", (ROOT / "include/constants/songs.h").read_text())[1])
    header = game.u32(game.sym["gSongTable"] + sound * 8)
    game.frames(1, A)
    trace, impact, shot = [], False, False
    for frame in range(360):
        state = player_state(game)
        npc = actors(game)[local_id]
        state.update({"frame": frame, "rival_position": list(npc["position"]), "rival_direction": npc["direction"]})
        trace.append(state)
        if state["facing_locked"]:
            assert state["direction"] == 2, state  # Backwards motion faces north.
        for player in ("gMPlayInfo_SE1", "gMPlayInfo_SE2"):
            song, status = struct.unpack("<II", game.read(game.sym[player], 8))
            impact |= song == header and bool(status & 0xFFFF) and not status & 0x80000000
        if state["jump_height"] < 0 and not shot:
            game.screenshot(name + "-shove")
            shot = True
        if game.read(game.sym["sGlobalScriptContext"] + 1, 1) == b"\0":
            break
        # Conflicting movement/menu inputs must not interrupt the cutscene.
        game.frames(1, LEFT | DOWN | B | START)
    else:
        raise AssertionError("Rival shove never released script control")
    game.frames(2)
    assert game.position() == (2, 5), trace[-1]
    assert actors(game)[local_id]["position"] == (3, 2) and actors(game)[local_id]["direction"] == 4
    assert any(row["position"] == [2, 3] for row in trace)
    assert any(row["rival_position"] == [2, 2] for row in trace)
    assert shot and impact, ("jump/impact not observed", shot, impact)
    state = player_state(game)
    assert not state["facing_locked"] and not state["frozen"] and state["jump_height"] == 0, state
    game.step(RIGHT)
    assert player_state(game)["direction"] == 4
    (game.output / (name + "-trace.json")).write_text(json.dumps(trace, indent=2) + "\n")
    print("PASS: rival encounter", name, flush=True)


def check_rival(game, maps, rows, checks):
    town, lab = maps["PolishedNewBarkTown"], maps["PolishedElmsLab"]
    local_id = resident_id(town, "LOCALID_NEW_BARK_RIVAL")
    game.restore("PolishedNewBarkTown")
    walk(game, town, (2, 2))
    npc = actors(game)[local_id]
    assert game.u32(game.sym["gObjectEventGraphicsInfoPointers"] + npc["graphics"] * 4) == game.sym["gObjectEventGraphicsInfo_PolishedRival"]
    for hour, period in ((7, "morn"), (10, "day"), (19, "eve"), (23, "nite")):
        game.set_hour(hour)
        check_resident_palette(game, town, local_id, 0, period, rows)
    rival_encounter(game, town, local_id, "rival-first")
    walk(game, town, (2, 2))
    rival_encounter(game, town, local_id, "rival-repeat")
    travel(game, town, lab)
    travel(game, lab, town)
    walk(game, town, (2, 2))
    rival_encounter(game, town, local_id, "rival-reentered")
    saved_map, saved_position = game.map_number(), game.position()
    save_game(game)
    continue_game(game)
    assert game.map_number() == saved_map and game.position() == saved_position, (saved_map, saved_position, game.map_number(), game.position())
    walk(game, town, (2, 2))
    rival_encounter(game, town, local_id, "rival-after-continue")
    walk(game, town, (2, 2))
    game.use_key_item(1)  # The demo supplies a Mach Bike before story progression.
    game.wait_player_images("sPicTable_PolishedChrisBike")
    rival_encounter(game, town, local_id, "rival-on-bike")
    assert game.read(game.sym["gPlayerAvatar"], 1)[0] & 2
    game.use_key_item(1)
    game.wait_player_images("sPicTable_PolishedChrisNormal")
    checks.append("rival uses authentic artwork and all four source palettes, with both source dialogue passages")
    checks.append("rival shove preserves north-facing backwards motion, a two-cell jump, impact sound and the source return path")
    checks.append("rival encounter ignores stray input, releases movement/facing locks, repeats after reentry and Save/Continue, and preserves bike state")


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
            check_residents(game, maps, rows, checks)
            check_rival(game, maps, rows, checks)
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
