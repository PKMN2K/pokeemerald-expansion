"""Climb Sprout Tower using controller input and read-only emulator probes."""
from collections import deque
from functools import lru_cache
import json
import re
import struct
import subprocess

from build_new_bark import ROOT, source_files
from polished_sprout import TRAINERS, PILLAR_TILES
from test_johto_demo_emulator import A, B, UP, DOWN, LEFT, RIGHT
from test_new_bark_emulator import map_data, actors, save_game, continue_game, close_resident_dialogue, player_state
from test_new_bark_intro_emulator import layout, advance, task_active, text_matches, party, script_active
from test_new_bark_battle_emulator import controller, choose_cell, check_portrait
from test_cherrygrove_emulator import map_id, flag, leave, interact
from test_violet_houses_emulator import enter, walk_city
from test_violet_mart_emulator import count, open_bag_item, open_shop, purchase, close_shop, stock
from test_academy_emulator import wait_for
from test_cherrygrove_emulator import money

DIRECTIONS = ((UP, 0, -1), (LEFT, -1, 0), (RIGHT, 1, 0), (DOWN, 0, 1))


@lru_cache
def constants():
    files = source_files()
    party_text = (ROOT / "src/data/polished_sprout.party").read_text()
    names = sorted(set(re.findall(r"\b(?:MOVE|TRAINER)_\w+", party_text)))
    names += ["SPECIES_BELLSPROUT", "SPECIES_HOOTHOOT", "SPECIES_GASTLY", "SPECIES_RATTATA"]
    names += ["MOVE_SURF", "MOVE_WATER_GUN", "MOVE_EMBER", "MOVE_BITE", "MOVE_RAZOR_LEAF"]
    names += ["ITEM_HM_FLASH", "ITEM_PARALYZE_HEAL", "ITEM_X_ACCURACY", "ITEM_POTION", "ITEM_POLISHED_ESCAPE_ROPE"]
    names += ["B_SCR_OP_YESNOBOXLEARNMOVE", "B_SCR_OP_YESNOBOXSTOPLEARNINGMOVE"]
    names += re.findall(r"^#define (FLAG_POLISHED_SPROUT_\w+)", (ROOT / "include/constants/flags.h").read_text(), re.M)
    names = sorted(set(names))
    code = '#include "global.h"\n#include "battle_setup.h"\n#include "constants/battle_script_commands.h"\n#include "constants/opponents.h"\n#include "constants/flags.h"\n#include "constants/items.h"\nconst u32 values[] = {' + ','.join(names) + ',offsetof(TrainerBattleParameter, params.opponentA)};\n'
    names.append("opponent_offset")
    r = subprocess.run(['arm-none-eabi-gcc', '-mcpu=arm7tdmi', '-mthumb', '-mabi=apcs-gnu', '-DMODERN=1', '-DEMERALD', '-std=gnu17', '-iquote', 'include', '-iquote', '.', '-S', '-x', 'c', '-o', '-', '-'], cwd=ROOT, input=code, text=True, capture_output=True, check=True)
    result = dict(zip(names, map(int, re.findall(r'\.word\s+(\d+)', r.stdout)), strict=True))
    result["ITEM_ESCAPE_ROPE"] = result["ITEM_POLISHED_ESCAPE_ROPE"]
    return result


def expected_party(name):
    files, c = source_files(), constants()
    kind = "Elder" if name == "Li" else "Sage"
    body = files["data/trainers/parties.asm"].decode().split(kind + "Group:\n", 1)[1].split('def_trainer ' + name.upper() + ', "' + name + '"\n', 1)[1].split("end_trainer")[0]
    attacks = files["data/pokemon/evos_attacks.asm"].decode()
    result = []
    for lv, species in re.findall(r"tr_mon (\d+), (\w+)", body):
        section = attacks.split("evos_attacks " + species.title() + "\n", 1)[1].split("evos_attacks ", 1)[0]
        moves = [c["MOVE_" + m] for level, m in re.findall(r"learnset (\d+), (\w+)", section) if int(level) <= int(lv)][-4:]
        result.append((c["SPECIES_" + species], int(lv), moves + [0] * (4 - len(moves))))
    return result


def settle(game, report):
    observed, portrait_seen, name = None, False, None
    attempted, last_slot, unavailable = None, None, set()
    for _ in range(3500):
        on_field = game.u32(game.sym["gMain"] + 4) == game.sym["CB2_Overworld"] | 1
        if on_field and not task_active(game, "Task_BattleStart") and not script_active(game):
            if observed is not None:
                outcome = game.read(game.sym["gBattleOutcome"], 1)[0]
                assert outcome == (1 if name else 4), (name, outcome, party(game, details=True))
                if name:
                    assert portrait_seen, name
                    assert flag(game, constants()["FLAG_POLISHED_SPROUT_" + name.upper()])
                    report["trainers"][name] = observed
                    game.snapshot("sprout-after-" + name.lower())
                    print("PASS: Sprout " + name + " source party, portrait and victory", flush=True)
                    heal(game)
                else:
                    report["wild"].append(observed[0])
            return
        if not on_field:
            trainer_id = int.from_bytes(game.read(game.sym["gTrainerBattleParameter"] + constants()["opponent_offset"], 2), "little")
            trainer = bool(game.u32(game.sym["gBattleTypeFlags"]) & 8)
            name = next((n for n in TRAINERS if constants()["TRAINER_POLISHED_SPROUT_" + n.upper()] == trainer_id), None) if trainer else None
            if name and not portrait_seen and observed is None:
                portrait_seen = check_portrait(game, "sprout-" + name.lower(), "elder" if name == "Li" else "sage")
        opcode = game.read(game.u32(game.sym["gBattlescriptCurrInstr"]), 1)[0] if not on_field else -1
        if opcode == constants()["B_SCR_OP_YESNOBOXLEARNMOVE"]:
            game.press(B, 20)  # Keep the existing moves and their PP.
        elif opcode == constants()["B_SCR_OP_YESNOBOXSTOPLEARNINGMOVE"]:
            game.press(A, 20)
        elif game.u32(game.sym["gMain"] + 4) == game.sym["CB2_UpdatePartyMenu"] | 1:
            mons = party(game, details=True)
            active = int.from_bytes(game.read(game.sym["gBattlerPartyIndexes"], 2), "little")
            if mons[active]["hp"]:
                game.press(B, 30)  # Cancel an optional shift prompt.
            else:
                healthy = next(i for i, m in enumerate(mons) if m["hp"])
                selected = game.read(game.sym["gPartyMenu"] + 9, 1)[0]
                game.press(A if selected == healthy else RIGHT if healthy else LEFT, 30)
        elif not on_field and controller(game, "HandleInputChooseAction"):
            if observed is None:
                observed = party(game, enemy=True, details=True)
                if name:
                    assert [(m["species"], m["level"], m["moves"]) for m in observed] == expected_party(name), (name, observed)
                    dv, ev = (26, 18) if name == "Li" else (24, 16)
                    assert all(m["ivs"] == [dv] * 6 and m["evs"] == [ev] * 6 and m["item"] == 0 for m in observed), observed
                    game.screenshot("sprout-" + name.lower() + "-battle")
                else:
                    c = constants()
                    assert len(observed) == 1 and observed[0]["species"] in [c["SPECIES_" + s] for s in ("RATTATA", "BELLSPROUT", "GASTLY")]
                    assert 3 <= observed[0]["level"] <= 7
                    game.screenshot("sprout-wild-" + str(len(report["wild"])))
            choose_cell(game, "gActionSelectionCursor", 0 if name else 3)
        elif not on_field and controller(game, "HandleInputChooseMove"):
            mon = party(game, details=True)[game.read(game.sym["gBattlerPartyIndexes"], 2)[0]]
            pp = game.read(game.sym["gBattleMons"] + layout()["battle_pp"], 4)
            enemy_index = int.from_bytes(game.read(game.sym["gBattlerPartyIndexes"] + 2, 2), "little")
            enemy = party(game, enemy=True, details=True)[enemy_index]
            signature = (tuple(pp), mon["hp"], mon["status"], enemy["hp"], enemy_index)
            if signature == attempted:
                unavailable.add(last_slot)  # A rejected choice, e.g. Gastly's Disable.
            else:
                unavailable.clear()
            strong_bite = enemy["species"] in (constants()["SPECIES_BELLSPROUT"], constants()["SPECIES_GASTLY"])
            preferred = [constants()["MOVE_" + m] for m in (("BITE", "EMBER", "SURF", "WATER_GUN", "RAZOR_LEAF", "VINE_WHIP") if strong_bite else ("SURF", "WATER_GUN", "BITE", "EMBER", "RAZOR_LEAF", "VINE_WHIP"))]
            slot = next((i for m in preferred for i, move in enumerate(mon["moves"]) if move == m and pp[i] and i not in unavailable), None)
            assert slot is not None, (name, mon)
            attempted, last_slot = signature, slot
            choose_cell(game, "gMoveSelectionCursor", slot)
        elif not on_field and controller(game, "PlayerHandleYesNoInput"):
            game.press(B, 20)
        else:
            game.press(A, 12)
    game.screenshot("sprout-stalled")
    game.snapshot("sprout-stalled")
    pointers = {key: game.u32(game.sym[key]) for key in ("gBattlerControllerFuncs", "gBattleMainFunc")}
    pointers["callback"] = game.u32(game.sym["gMain"] + 4)
    resolved = {key: [n for n, value in game.sym.items() if isinstance(value, int) and value | 1 == ptr] for key, ptr in pointers.items()}
    raise AssertionError(("Sprout interaction stalled", name, game.position(), resolved, party(game, details=True)))


def heal(game):
    mon = party(game, details=True)[0]
    if not 0 < mon["hp"] <= mon["max_hp"] * 0.65:
        return
    item = constants()["ITEM_POTION"]
    assert count(game, item), ("Need a Potion for the climb", mon)
    open_bag_item(game, item)
    game.press(A, 120)
    wait_for(game, lambda: task_active(game, "Task_HandleChooseMonInput"), "Potion party selection")
    for _ in range(3):
        if game.read(game.sym["gPartyMenu"] + 9, 1)[0] == 0:
            break
        game.press(LEFT, 20)
    game.press(A, 90)
    wait_for(game, lambda: party(game, details=True)[0]["hp"] > mon["hp"], "Potion recovery")
    for _ in range(30):
        # Closing the Bag returns through the Start menu, which already uses
        # CB2_Overworld. Wait until that menu releases the player as well.
        if (game.u32(game.sym["gMain"] + 4) == game.sym["CB2_Overworld"] | 1
                and not script_active(game) and not task_active(game, "Task_ShowStartMenu")
                and not player_state(game)["frozen"]):
            return
        game.press(B, 45)
    raise AssertionError("Potion did not return to the field")


def path(game, data, goal, allow_scene=False):
    blocked = {(w["x"], w["y"]) for w in data["warp_events"]}
    live = actors(game)
    for number, obj in enumerate(data["object_events"], 1):
        if obj["flag"] != "0" and flag(game, constants()[obj["flag"]]):
            continue
        x, y = live[number]["position"] if number in live else (obj["x"], obj["y"])
        blocked.add((x, y))
    if (data["name"] == "PolishedSproutTower3F" and not allow_scene
            and not flag(game, constants()["FLAG_POLISHED_SPROUT_RIVAL_LEFT"])):
        blocked.add((9, 9))
    blocked.discard(goal)
    start = game.position()
    queue, paths = deque([start]), {start: []}
    while queue:
        x, y = queue.popleft()
        if (x, y) == goal:
            return paths[x, y]
        for key, dx, dy in DIRECTIONS:
            pos = x + dx, y + dy
            if pos in paths or pos in blocked or not (0 <= pos[0] < 16 and 0 <= pos[1] < 16):
                continue
            if data["cells"][pos[1] * 16 + pos[0]] & 0xc00:
                continue
            paths[pos] = paths[x, y] + [key]
            queue.append(pos)
    raise AssertionError(("No Sprout path", data["name"], start, goal))


def walk(game, data, goal, report, allow_scene=False):
    for _ in range(250):
        if game.map_number() != map_id(game, data):
            return
        if game.position() == goal:
            return
        settle(game, report)
        keys = path(game, data, goal, allow_scene)
        try:
            game.step(keys[0])
        except AssertionError as error:
            if not str(error).startswith("Blocked at"):
                raise
            settle(game, report)
            game.frames(30)
        game.frames(45)
        if game.position() == goal and allow_scene:
            return
    raise AssertionError(("Sprout walk stalled", data["name"], goal, game.position()))


def stair(game, data, index, target, report):
    w = data["warp_events"][index]
    goal = w["x"], w["y"]
    if game.position() == goal:
        for _, dx, dy in DIRECTIONS:
            neighbor = goal[0] + dx, goal[1] + dy
            if 0 <= neighbor[0] < 16 and 0 <= neighbor[1] < 16 and not data["cells"][neighbor[1] * 16 + neighbor[0]] & 0xc00:
                walk(game, data, neighbor, report)
                break
    walk(game, data, goal, report)
    game.frames(240)
    assert game.map_number() == map_id(game, target), (data["name"], index, game.map_number(), game.position())
    dest = target["warp_events"][int(w["dest_warp_id"])]
    assert game.position() == (dest["x"], dest["y"]), (w, game.position())
    game.screenshot("sprout-stair-" + data["name"] + "-" + str(index))
    report["stairs"].append([data["name"], index, target["name"]])


def interact_object(game, data, number, report):
    obj = data["object_events"][number - 1]
    x, y = actors(game).get(number, {}).get("position", (obj["x"], obj["y"]))
    for key, dx, dy in DIRECTIONS:
        goal = x - dx, y - dy
        if not (0 <= goal[0] < 16 and 0 <= goal[1] < 16) or data["cells"][goal[1] * 16 + goal[0]] & 0xc00:
            continue
        try:
            path(game, data, goal)
        except AssertionError:
            continue
        walk(game, data, goal, report)
        game.face(key)
        game.press(A, 30)
        settle(game, report)
        return
    raise AssertionError(("No approach to Sprout object", data["name"], number))


def take(game, data, item, report):
    number, obj = next((n, o) for n, o in enumerate(data["object_events"], 1) if o["script"].endswith("_" + item))
    before = count(game, constants()["ITEM_" + item])
    interact_object(game, data, number, report)
    assert flag(game, constants()[obj["flag"]]) and count(game, constants()["ITEM_" + item]) == before + 1, item
    report["items"].append(item)


def pillar(game):
    raw = (ROOT / "graphics/polished_sprout/pillar.bin").read_bytes()
    bg = int.from_bytes(game.read(0x0400000c, 2), "little")
    base = 0x06000000 + ((bg >> 2) & 3) * 0x4000
    observed = [set() for _ in PILLAR_TILES]
    for _ in range(160):
        game.frames(1)
        for n, tile in enumerate(PILLAR_TILES):
            frame = game.read(base + (tile + 1) * 32, 32)
            choices = [raw[(n * 5 + f) * 32:(n * 5 + f + 1) * 32] for f in range(5)]
            assert frame in choices, (n, frame.hex())
            observed[n].add(frame)
    assert all(len(seen) == len({raw[(n * 5 + f) * 32:(n * 5 + f + 1) * 32] for f in range(5)}) for n, seen in enumerate(observed))


def check_sprout(game, city, checks):
    report = {"trainers": {}, "wild": [], "stairs": [], "items": []}
    floors = [map_data("PolishedSproutTower" + str(n) + "F") for n in (1, 2, 3)]
    one, two, three = floors
    game.restore("violet-center")
    game.set_hour(12)
    center = map_data("PolishedVioletPokeCenter1F")
    interact(game, center, (5, 3), UP)
    advance(game, lambda: not script_active(game), limit=400)
    leave(game, center, city)
    mart = map_data("PolishedVioletMart")
    enter(game, city, mart)
    open_shop(game, mart)
    potion = stock(game)[2]
    for n in range(min(5, money(game) // 300)):
        purchase(game, 2 if n == 0 else 0, potion)
    close_shop(game)
    leave(game, mart, city)
    enter(game, city, one)
    assert game.position() == (7, 15)
    game.snapshot("sprout-entry")
    game.screenshot("sprout-1f")
    pillar(game)
    for number in (1, 2, 4, 5):
        interact_object(game, one, number, report)
    checks.append("Sprout Tower opens from Violet; its ten pillar tiles cycle through the exact source frames and first-floor residents can be read")
    stair(game, one, 2, two, report)
    interact_object(game, two, 1, report)
    stair(game, two, 2, one, report)
    take(game, one, "PARALYZE_HEAL", report)
    interact_object(game, one, 6, report)
    stair(game, one, 3, two, report)
    take(game, two, "X_ACCURACY", report)
    interact_object(game, two, 2, report)
    stair(game, two, 3, three, report)
    game.screenshot("sprout-3f")
    take(game, three, "POTION", report)
    for number in (2, 4, 3):
        interact_object(game, three, number, report)
    game.snapshot("sprout-before-summit")
    walk(game, three, (9, 9), report, allow_scene=True)
    advance(game, lambda: text_matches(game, "PolishedSproutTower3F_Lecture"), limit=400)
    assert game.position() == (9, 5) and actors(game)[1]["position"] == (8, 3)
    game.screenshot("sprout-rival-lecture")
    advance(game, lambda: text_matches(game, "PolishedSproutTower3F_RivalText"))
    assert actors(game)[1]["position"] == (9, 4)
    game.screenshot("sprout-rival")
    advance(game, lambda: text_matches(game, "PolishedSproutTower3F_Escape"))
    advance(game, lambda: not script_active(game), limit=400)
    assert flag(game, constants()["FLAG_POLISHED_SPROUT_RIVAL_LEFT"]) and 1 not in actors(game)
    take(game, three, "ESCAPE_ROPE", report)
    before = count(game, constants()["ITEM_HM_FLASH"])
    interact_object(game, three, 5, report)
    assert flag(game, constants()["FLAG_POLISHED_SPROUT_FLASH"])
    assert count(game, constants()["ITEM_HM_FLASH"]) == before + 1
    assert set(report["trainers"]) == set(TRAINERS), report
    game.screenshot("sprout-flash-received")
    save_game(game)
    continue_game(game)
    assert game.map_number() == map_id(game, three) and 1 not in actors(game)
    for suffix in ("RIVAL_LEFT", "FLASH", *(n.upper() for n in TRAINERS)):
        assert flag(game, constants()["FLAG_POLISHED_SPROUT_" + suffix]), suffix
    interact_object(game, three, 5, report)
    assert count(game, constants()["ITEM_HM_FLASH"]) == before + 1
    walk(game, three, (9, 9), report, allow_scene=True)
    game.frames(200)
    assert not script_active(game)
    checks.append("All six Sages and Elder Li use source parties, moves, converted DVs/EVs and portraits; victories, Rival departure and one Flash reward survive Save/Continue")
    checks.append("The summit follows the source player and Rival paths, shows the Elder's lecture and removes the Rival after his Escape Rope")
    game.set_hour(23)
    start = len(report["wild"])
    for _ in range(800):
        walk(game, three, (9, 8) if game.position() == (9, 9) else (9, 9), report)
        settle(game, report)
        if len(report["wild"]) > start:
            break
    assert len(report["wild"]) > start, "No nighttime tower encounter"
    night = report["wild"][-1]
    expected = {("RATTATA", 4), ("RATTATA", 5), ("GASTLY", 6), ("GASTLY", 7),
                ("BELLSPROUT", 4), ("BELLSPROUT", 6), ("BELLSPROUT", 7)}
    assert (night["species"], night["level"]) in {(constants()["SPECIES_" + s], lv) for s, lv in expected}
    report["night_encounter"] = night
    checks.append("A real 23:00 summit encounter uses the source nighttime species and level table and permits native fleeing")
    game.set_hour(12)
    game.snapshot("sprout-summit-cleared")
    rope = constants()["ITEM_ESCAPE_ROPE"]
    before_rope = count(game, rope)
    open_bag_item(game, rope)
    game.press(A, 180)
    advance(game, lambda: game.map_number() == map_id(game, city)
            and game.u32(game.sym["gMain"] + 4) == game.sym["CB2_Overworld"] | 1
            and not script_active(game), limit=400)
    game.frames(200)
    assert game.position() == (23, 2) and count(game, rope) == before_rope - 1
    game.screenshot("sprout-escape-rope")
    checks.append("Using the real Escape Rope from the summit returns to Violet's tower entrance and consumes exactly one rope")
    game.restore("sprout-summit-cleared")
    stair(game, three, 0, two, report)
    stair(game, two, 1, one, report)
    stair(game, one, 4, two, report)
    stair(game, two, 0, one, report)
    game.snapshot("sprout-returned-1f")
    for index in (0, 1):
        game.restore("sprout-returned-1f")
        leave(game, one, city, index)
        assert game.position() == (23, 2)
    checks.append("All four pickups are added to the Bag; each internal stair pair and both tower exits return to the correct source tile")
    (game.output / "sprout-validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: Sprout Tower climb, summit, seven victories, rewards and Save/Continue", flush=True)
