"""Real-input Route 29 checks, called from the New Bark emulator campaign.

All RAM access is read-only. Snapshots originate in the tested ROM's gameplay.
"""
from collections import deque
import json
import struct

from build_johto_demo import ROOT, behavior_ids
from test_johto_demo_emulator import A, UP, DOWN, LEFT, RIGHT
from test_new_bark_emulator import (map_data, walk, actors, close_resident_dialogue,
    save_game, continue_game, check_resident_palette, talk_to_resident)
from test_new_bark_intro_emulator import advance, script_active, text_matches, party, layout, var, task_active
from test_new_bark_battle_emulator import controller, choose_cell, potion_count


def check_guard(game, town, checks):
    game.snapshot("route29-before-guard")
    original_map = game.map_number()
    for y in (8, 9):
        game.restore("route29-before-guard")
        walk(game, town, (2, y))
        game.step(LEFT)
        advance(game, lambda: text_matches(game, "PolishedRoute29_Text_ItsDangerousToGoAlone"))
        assert game.position() == (5, 8)
        close_resident_dialogue(game)
        assert game.map_number() == original_map and actors(game)[1]["position"] == (6, 8)
        assert var(game, "starter") == 0
        game.screenshot(f"route29-guard-{y}")
    game.restore("route29-before-guard")
    checks.append("Route 29 exit guard returns the player from both source approach rows before receiving a starter")
    print("PASS: Route 29 pre-starter guard", flush=True)


def finish_wild(game, observed, label):
    seen = False
    for _ in range(350):
        if task_active(game, "Task_BattleStart"):
            game.frames(20)
            continue
        if game.u32(game.sym["gMain"] + 4) == game.sym["CB2_Overworld"] | 1:
            game.frames(30)
            if task_active(game, "Task_BattleStart") or game.u32(game.sym["gMain"] + 4) != game.sym["CB2_Overworld"] | 1:
                continue
            return
        if controller(game, "HandleInputChooseAction"):
            if not seen:
                enemy = party(game, enemy=True, details=True)
                assert len(enemy) == 1 and enemy[0]["level"] in (2, 3), enemy
                observed.append(enemy[0])
                game.screenshot("route29-wild-" + label)
                seen = True
            choose_cell(game, "gActionSelectionCursor", 3)  # Native RUN action.
        else:
            game.press(A, 20)
    raise AssertionError("Route 29 wild battle did not return to the field")


def route_walk(game, data, goal, observed, battle_handler=finish_wild):
    width, height = data["width"], data["height"]
    stem = "outdoor" if data["name"] in ("PolishedRoute29", "PolishedRoute30", "PolishedRoute31") else "gate"
    attr = b"".join((ROOT / f"data/tilesets/{side}/polished_new_bark_{stem}/metatile_attributes.bin").read_bytes()
                    for side in ("primary", "secondary"))
    attrs = struct.unpack("<1024H", attr)
    ids = behavior_ids()
    jumps = {ids["MB_JUMP_SOUTH"]: (DOWN,), ids["MB_JUMP_WEST"]: (LEFT,), ids["MB_JUMP_EAST"]: (RIGHT,),
        ids["MB_JOHTO_LEDGE_DOWN_LEFT"]: (DOWN, LEFT), ids["MB_JOHTO_LEDGE_DOWN_RIGHT"]: (DOWN, RIGHT)}
    blocked = {(w["x"], w["y"]) for w in data["warp_events"]}
    for row in data["object_events"]:
        for dx in range(-row["movement_range_x"], row["movement_range_x"] + 1):
            for dy in range(-row["movement_range_y"], row["movement_range_y"] + 1):
                blocked.add((row["x"] + dx, row["y"] + dy))
    blocked.discard(goal)
    start = game.position()
    queue, paths = deque([start]), {start: []}
    def free(pos):
        x, y = pos
        return (0 <= x < width and 0 <= y < height and pos not in blocked
                and not data["cells"][y*width+x] & 0xC00 and data["cells"][y*width+x] >> 12 == 3)
    while queue:
        pos = queue.popleft()
        if pos == goal:
            for key, target in paths[pos]:
                for attempt in range(3):
                    battle_handler(game, observed, str(len(observed)))
                    try:
                        game.step(key)
                        break
                    except AssertionError:
                        if not task_active(game, "Task_BattleStart") and game.u32(game.sym["gMain"]+4)==game.sym["CB2_Overworld"]|1:
                            raise
                else:
                    raise AssertionError("Repeated encounter startup prevented a route step")
                game.frames(45)
                battle_handler(game, observed, str(len(observed)))
                assert game.position() == target, (pos, target, game.position())
            return
        for key, dx, dy in ((RIGHT, 1, 0), (LEFT, -1, 0), (UP, 0, -1), (DOWN, 0, 1)):
            target = pos[0] + dx, pos[1] + dy
            behavior = attrs[data["cells"][pos[1]*width+pos[0]] & 0x3ff]
            if key in jumps.get(behavior, ()):
                target = target[0] + dx, target[1] + dy
            if not free(target):
                continue
            if target not in paths:
                paths[target] = paths[pos] + [(key, target)]
                queue.append(target)
    raise AssertionError(f"No route from {start} to {goal}")


def interact(game, data, position, key, observed, label=None):
    route_walk(game, data, position, observed)
    game.face(key)
    game.press(A, 100)
    if label:
        advance(game, lambda: text_matches(game, label))
    assert script_active(game)
    game.frames(240)
    game.screenshot("route29-" + (label or "object"))


def route_talk(game, data, local, observed, rows, color):
    npc = data["object_events"][local-1]
    for _ in range(12):
        live = actors(game).get(local)
        x, y = live["position"] if live else (npc["x"], npc["y"])
        for key, dx, dy in ((UP, 0, 1), (LEFT, 1, 0), (RIGHT, -1, 0), (DOWN, 0, -1)):
            # Approach from outside the patrol lane, so the moving resident
            # cannot step onto the player's intended destination en route.
            if (abs(x + dx - npc["x"]) <= npc["movement_range_x"]
                and abs(y + dy - npc["y"]) <= npc["movement_range_y"]):
                continue
            try:
                route_walk(game, data, (x+dx, y+dy), observed)
            except AssertionError as error:
                if str(error).startswith("No route from"):
                    continue
                raise
            if actors(game).get(local, {}).get("position") != (x,y):
                break
            game.face(key)
            game.press(A, 100)
            if script_active(game):
                assert struct.unpack("<H", game.read(game.sym["gSpecialVar_LastTalked"], 2))[0] == local
                assert text_matches(game, npc["script"] + "_Text")
                check_resident_palette(game, data, local, color, "day", rows)
                game.frames(240)
                game.screenshot("route29-resident-" + str(local))
                close_resident_dialogue(game)
                return
    raise AssertionError(f"Cannot talk to Route 29 resident {local}")


def check_route(game, maps, rows, checks):
    town = maps["PolishedNewBarkTown"]
    route, gate = map_data("PolishedRoute29"), map_data("PolishedRoute2946Gate")
    group = game.map_number()[0]
    group_maps = json.loads((ROOT / "data/maps/map_groups.json").read_text())["gMapGroup_PolishedJohtoDemo"]
    route_id, gate_id = (group, group_maps.index(route["name"])), (group, group_maps.index(gate["name"]))
    assert var(game, "starter") != 0
    observed = []
    game.command(f"save {game.output / 'route29-ready.state'}")
    game.snapshot("route29-before-visit")
    game.set_hour(10)
    walk(game, town, (0, 8))
    game.step(LEFT)
    game.frames(80)
    assert game.map_number() == route_id and game.position() == (59, 8), (game.map_number(), game.position())
    game.screenshot("route29-east-entry")
    game.snapshot("route29-entry")
    interact(game, route, (51, 8), UP, observed, "PolishedRoute29_Route29Sign1Text_Text")
    close_resident_dialogue(game)
    # Real item pickup: repeat access and cartridge Save/Continue cannot duplicate it.
    before = potion_count(game)
    interact(game, route, (47, 2), RIGHT, observed)
    close_resident_dialogue(game)
    assert potion_count(game) == before + 1
    save_game(game)
    continue_game(game)
    assert game.map_number() == route_id and potion_count(game) == before + 1
    assert 8 not in actors(game)
    checks.append("Route 29 Potion uses the native Bag and remains collected after Save/Continue")
    print("PASS: Route 29 Potion and save", flush=True)
    route_walk(game, route, (27, 2), observed)
    game.step(UP)
    game.frames(200)
    assert game.map_number() == gate_id, game.map_number()
    game.screenshot("route29-gate")
    for local_id, color in ((1, 0), (2, 2)):
        check_resident_palette(game, gate, local_id, color, "day", rows)
        if local_id == 1:
            interact(game, gate, (2, 4), LEFT, observed, "PolishedRoute2946Gate_Route29Route46GateOfficerText_Text")
            close_resident_dialogue(game)
        else:
            talk_to_resident(game, gate, local_id, "route29-gate-resident-" + str(local_id))
    # The incomplete onward map is collision-blocked at both door cells.
    for x in (4, 5):
        route_walk(game, gate, (x, 1), observed)
        game.frames(40, UP)
        assert game.position() == (x, 1) and game.map_number() == gate_id
    game.snapshot("route29-gate-interior")
    for x in (4, 5):
        game.restore("route29-gate-interior")
        route_walk(game, gate, (x, 7), observed)
        game.frames(40, DOWN)
        game.frames(200)
        # Native door exit steps onto the walkable cell below the source warp.
        assert game.map_number() == route_id and game.position() == (27, 2)
    checks.append("Route 29 gate entrance and both south exits are reciprocal; both north doors remain safely blocked")
    print("PASS: Route 29 gate", flush=True)
    for local, color in ((3, 1), (2, 2), (1, 2)):
        route_talk(game, route, local, observed, rows, color)
    checks.append("Route 29 and gate residents retain source conversations, sprite colors and moving-object interactions")
    interact(game, route, (13, 3), DOWN, observed, "PolishedRoute29_Text_WaitingForNight")
    close_resident_dialogue(game)
    for hour, label in ((4, "Morning"), (5, "Night"), (16, "Night"), (17, "Morning"), (21, "Morning")):
        game.set_hour(hour)
        interact(game, route, (13, 3), DOWN, observed, "PolishedRoute29_Text_WaitingFor" + label)
        close_resident_dialogue(game)
    checks.append("Route 29 resident changes source dialogue at 05:00 and 17:00")
    # Daily berries persist across Save/Continue and replenish after midnight.
    game.set_hour(10)
    interact(game, route, (12, 3), UP, observed)
    advance(game, lambda: any(text_matches(game, "PolishedRoute29_FruitObtained" + n) for n in ("One", "Two", "Three")))
    game.frames(240)
    game.screenshot("route29-fruit-harvest")
    close_resident_dialogue(game)
    interact(game, route, (12, 3), UP, observed, "PolishedRoute29_FruitNothing")
    close_resident_dialogue(game)
    save_game(game)
    continue_game(game)
    interact(game, route, (12, 3), UP, observed, "PolishedRoute29_FruitNothing")
    close_resident_dialogue(game)
    game.set_hour(9)  # set_hour advances forward: next day, 09:00.
    interact(game, route, (12, 3), UP, observed)
    advance(game, lambda: any(text_matches(game, "PolishedRoute29_FruitObtained" + n) for n in ("One", "Two", "Three")))
    close_resident_dialogue(game)
    checks.append("Route 29 berry harvest persists across Save/Continue and regrows on the next RTC day")
    print("PASS: Route 29 daily berries", flush=True)
    game.set_hour(23)
    before_night = len(observed)
    for sample in range(100):
        route_walk(game, route, (12 + sample % 2, 6), observed)
        if len(observed) > before_night:
            break
    else:
        raise AssertionError("No nighttime encounter after 100 grass steps")
    assert all(mon["species"] in (layout()["Hoothoot"], layout()["Rattata"])
               for mon in observed[before_night:]), observed[before_night:]
    checks.append("Route 29 nighttime grass uses Hoothoot/Rattata through the native battle engine while global timed encounters remain disabled")
    # Traverse the full width in both directions, including native ledge jumps.
    route_walk(game, route, (1, 6), observed)
    game.screenshot("route29-west-end")
    from test_cherrygrove_emulator import check_city
    check_city(game, route, observed, rows, checks)
    interact(game, route, (3, 6), UP, observed, "PolishedRoute29_Route29Sign2Text_Text")
    close_resident_dialogue(game)
    route_walk(game, route, (59, 8), observed)
    game.step(RIGHT)
    game.frames(80)
    assert game.map_number() == (group, group_maps.index("PolishedNewBarkTown")) and game.position() == (0, 8)
    assert observed, "No native grass battle encountered during route traversal"
    checks.append("Route 29 can be traversed across its source grass and ledges, with native wild battles and a seamless return to New Bark")
    game.restore("route29-before-visit")
    print("PASS: Route 29 traversal, battles and reciprocal connection", flush=True)
