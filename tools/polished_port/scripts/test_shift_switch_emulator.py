#!/usr/bin/env python3
"""Drive source/native Shift and wild faint prompts with real input.

The driver selects battle styles through Options, reloads cartridge Flash and
checks the resulting prompt. It supplies buttons and reads production state.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from build_pokemon_learnsets import PIN
from test_johto_demo_emulator import Game, ROOT, A, B, DOWN, LEFT, RIGHT
from test_new_bark_battle_emulator import controller, choose_cell
from test_new_bark_intro_emulator import task_active


LAYOUT = ("battle_size", "species", "hp", "max_hp", "stages", "volatiles",
          "volatile_size", "party_size", "party_status", "party_slot",
          "yesno_opcode", "attack", "chansey", "arcanine", "skarmory", "ninetales",
          "party_hp", "outcome_ran", "outcome_lost", "active_gimmicks",
          "player_trainer", "opponent_trainer", "party_count", "gimmick_none", "fade_size", "battle_style")

STYLE_BITS = {"shift": 0, "switch": 0, "set": 1 << 9, "predict": 1 << 12}
STYLE_IDS = {"shift": 0, "switch": 0, "set": 1, "predict": 2}
ENGLISH = {int(value, 16): char for char, value in re.findall(
    r"^'(.)'\s*=\s*([0-9A-F]{2})", (ROOT / "charmap.txt").read_text().split("@ Hiragana")[0], re.M)}
ENGLISH.update(dict.fromkeys((0x39, 0xFA, 0xFB, 0xFE), " "))  # Nonbreaking space and dialogue breaks

PROFILES = {
    "source": dict(stem="pokeemerald-johto-demo", build="emerald-johto-demo", demo=1),
    "native": dict(stem="pokeemerald", build="emerald", demo=0),
}


def compile_shift_probe(directory, code, profile):
    """Link only this fixture against the selected shipping object's profile.

    Keep audit_moves_machines.compile_probe and all of its callers unchanged.
    The native control uses native objects and declarations, not source objects
    with a different Python expectation.
    """
    selected = PROFILES[profile]
    map_path = ROOT / (selected["stem"] + ".map")
    rom_path = ROOT / (selected["stem"] + ".gba")
    assert map_path.is_file() and rom_path.is_file(), f"Build the {profile} production ROM first"
    c, obj, elf, linker = (directory / name for name in ("probe.c", "probe.o", "probe.elf", "probe.ld"))
    c.write_text(code)
    subprocess.run(["arm-none-eabi-gcc", "-c", "-Wa,-meabi=5", "-mthumb",
        "-mthumb-interwork", "-O2", "-mabi=apcs-gnu", "-march=armv4t", "-std=gnu17",
        "-DMODERN=1", "-DTESTING=0", "-DEMERALD",
        f"-DPOLISHED_JOHTO_DEMO={selected['demo']}", "-DPOLISHED_MOVE_BALANCE_TEST=0",
        "-Wall", "-Werror", "-iquote", "include", "-iquote", ".", str(c), "-o", str(obj)],
        cwd=ROOT, check=True)
    linker.write_text((ROOT / "ld_script_modern.ld").read_text().replace(
        "gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;",
        "gInitialMainCB2 = CB2_MoveMachineAudit;"))
    objects = [line[5:] for line in map_path.read_text().splitlines()
        if line.startswith("LOAD ") and line != "LOAD linker stubs"]
    assert objects, f"No production object list in {map_path}"
    subprocess.run(["arm-none-eabi-ld", "--gc-sections", "-T", str(linker),
        "-o", str(elf), str(obj), *objects], cwd=ROOT / "build" / selected["build"], check=True)
    subprocess.run([str(ROOT / "tools/gbafix/gbafix"), str(elf), "-tPOKEMON EMER",
        "-cBPEE", "-m01", "-r0", "-d0", "--silent"], check=True)
    return elf


class Observer:
    def __init__(self, game, source):
        self.game = game
        self.compiled_source_rules = game.u32(game.sym["gShiftSwitchProbeSourceRules"])
        assert self.compiled_source_rules == int(source), (
            "fixture was compiled for the wrong rule profile", self.compiled_source_rules, source)
        self.layout = dict(zip(LAYOUT, struct.unpack("<" + "I" * len(LAYOUT),
            game.read(game.sym["gShiftSwitchProbeLayout"], 4 * len(LAYOUT)))))
        self.mask = game.read(game.sym["gShiftSwitchTrapMask"], self.layout["volatile_size"])
        self.fade_mask = game.read(game.sym["gShiftSwitchFadeMask"], self.layout["fade_size"])
        self.events = []
        self.last_faint_state = None
        self.previous = self.mons()

    def mons(self):
        c = self.layout
        raw = self.game.read(self.game.sym["gBattleMons"], c["battle_size"] * 2)
        result = []
        for side in range(2):
            mon = raw[side * c["battle_size"]:(side + 1) * c["battle_size"]]
            value = lambda name: struct.unpack_from("<H", mon, c[name])[0]
            volatile = mon[c["volatiles"]:c["volatiles"] + c["volatile_size"]]
            result.append(dict(species=value("species"), hp=value("hp"), max_hp=value("max_hp"),
                attack=mon[c["stages"] + c["attack"]],
                trapped=any(a & b for a, b in zip(volatile, self.mask))))
        return result

    def observe(self):
        current = self.mons()
        for side, name in ((0, "player"), (1, "opponent")):
            old, new = self.previous[side], current[side]
            incoming = self.layout["arcanine" if side == 0 else "ninetales"]
            if new["species"] == incoming:
                if old["species"] != incoming:
                    self.events.append("send_" + name)
                else:
                    if new["hp"] < old["hp"]:
                        self.events.append("hazard_" + name)
                    if new["attack"] < old["attack"]:
                        self.events.append("intimidate_" + name)
        # Terminal cleanup clears the enemy party during the fade and then
        # zeroes gBattleMons. Retain the final state with original identities;
        # a later foe faint must replace an earlier surviving-foe observation.
        if (current[0]["species"] == self.layout["chansey"] and current[0]["hp"] == 0
                and current[1]["species"] == self.layout["skarmory"]):
            self.last_faint_state = current
        self.previous = current
        return current

    def tick(self, key=0):
        self.game.frames(1, key)
        return self.observe()

    def action_menu(self):
        return controller(self.game, "HandleInputChooseAction")

    def party_menu(self):
        # This task is installed before the fade finishes, and its actual
        # button handler ignores input while gPaletteFade.active is set.
        return task_active(self.game, "Task_HandleChooseMonInput") and not any(a & b for a, b in
            zip(self.game.read(self.game.sym["gPaletteFade"], self.layout["fade_size"]), self.fade_mask))

    def prompt(self):
        pointer = self.game.u32(self.game.sym["gBattlescriptCurrInstr"])
        return (0x08000000 <= pointer < 0x0A000000
            and self.game.read(pointer, 1)[0] == self.layout["yesno_opcode"]
            and self.game.read(self.game.sym["gBattleCommunication"], 1)[0] == 1)

    def wait(self, predicate, label, advance=False, limit=6000):
        for frame in range(limit):
            if predicate():
                return
            # Acknowledging faint/experience text must not answer an unexpected
            # Yes/No question or select a party member on the observer's behalf.
            key = A if advance and frame % 24 == 0 and not self.prompt() and not task_active(
                self.game, "Task_HandleChooseMonInput") else 0
            self.tick(key)
        raise AssertionError((label, self.events, self.mons()))

    def key(self, key):
        self.tick(key)
        self.tick()

    def indexes(self):
        return struct.unpack("<2H", self.game.read(self.game.sym["gBattlerPartyIndexes"], 4))

    def assert_no_gimmicks(self):
        c = self.layout
        base = self.game.u32(self.game.sym["gBattleStruct"]) + c["active_gimmicks"]
        actual = [self.game.read(base + trainer * c["party_count"] + index, 1)[0]
            for trainer, index in zip((c["player_trainer"], c["opponent_trainer"]), self.indexes())]
        assert actual == [c["gimmick_none"]] * 2, ("ordinary probe activated a gimmick", actual, self.mons())
        return actual

    def assert_initial_setup(self, wild=False):
        mons = self.mons()
        assert self.indexes() == (0, 0), ("unexpected initial party slots", self.indexes())
        assert mons[0]["species"] == self.layout["chansey"] and mons[0]["hp"] == 160, mons
        assert mons[1]["species"] == self.layout["skarmory"], mons
        expected_hp, expected_max = (1000, 1000) if wild else (40, 160)
        assert (mons[1]["hp"], mons[1]["max_hp"]) == (expected_hp, expected_max), (
            "live setup HP changed before the first input", mons)
        self.setup = dict(initial_mons=mons, initial_gimmicks=self.assert_no_gimmicks())

    def lead_status(self):
        return self.game.u32(self.game.sym["gParties"] + self.layout["party_status"])

    def lead_hp(self):
        return self.party_hp(self.layout["player_trainer"])

    def party_hp(self, trainer, slot=0):
        c = self.layout
        address = self.game.sym["gParties"] + (trainer * c["party_count"] + slot) * c["party_size"] + c["party_hp"]
        return struct.unpack("<H", self.game.read(address, 2))[0]

    def outcome(self):
        return self.game.read(self.game.sym["gBattleOutcome"], 1)[0]

    def finished(self):
        return self.game.u32(self.game.sym["gMain"] + 4) == self.game.sym["CB2_ShiftSwitchDone"] | 1

    def reset_observation(self):
        self.events.clear()
        self.previous = self.mons()

    def choose_reserve(self, expected_indexes=(1, 1)):
        self.wait(self.party_menu, "optional party menu")
        # Party controllers may open the usual action submenu after choosing
        # the row. All input is genuine; the result is never injected.
        for frame in range(6000):
            if self.action_menu() and self.indexes() == expected_indexes:
                return
            key = 0
            if frame % 20 == 0:
                if self.party_menu():
                    slot = self.game.read(self.game.sym["gPartyMenu"] + self.layout["party_slot"], 1)[0]
                    key = A if slot == 1 else RIGHT
                else:
                    key = A
            self.tick(key)
        raise AssertionError(("accepted switch did not reach next action", self.events, self.indexes()))

    def assert_entries(self, source, trap=False):
        self.assert_no_gimmicks()
        events = self.events
        wanted = ["send_opponent", "send_player"] if source else ["send_player", "send_opponent"]
        sends = [event for event in events if event.startswith("send_")]
        assert sends == wanted, ("departed Speed order", source, events)
        assert events.count("hazard_opponent") == 1, events
        assert events.count("intimidate_opponent") == 1, events
        hazard, ability = events.index("hazard_opponent"), events.index("intimidate_opponent")
        assert (hazard < ability) == source, ("both hazards before ability", source, events)
        if not trap:
            assert events.count("hazard_player") == 1, events
            assert events.index("hazard_player") < ability, events
        assert self.indexes() == (1, 1)
        assert self.lead_status() == 0, "departing Natural Cure did not cure the correct party slot"
        assert not self.mons()[0]["trapped"], "replacement inherited the departed Mean Look trap"
        assert self.game.read(self.game.sym["gSentPokesToOpponent"], 1)[0] == 2, "withdrawn living mon counted as a new foe participant"


def prepare(game, observer, expect_prompt):
    observer.wait(observer.action_menu, "initial action menu", advance=True)
    observer.assert_initial_setup()
    choose_cell(game, "gActionSelectionCursor", 0)
    observer.wait(lambda: controller(game, "HandleInputChooseMove"), "first move menu")
    choose_cell(game, "gMoveSelectionCursor", 0)  # Spikes; foe Spikes or Mean Look.
    observer.wait(observer.action_menu, "second action menu")
    before_ko = observer.mons()
    assert (before_ko[1]["hp"], before_ko[1]["max_hp"]) == (40, 160), (
        "foe must still have 40 HP before Dragon Rage", before_ko)
    observer.setup.update(before_ko_mons=before_ko, before_ko_gimmicks=observer.assert_no_gimmicks())
    choose_cell(game, "gActionSelectionCursor", 0)
    observer.wait(lambda: controller(game, "HandleInputChooseMove"), "second move menu")
    choose_cell(game, "gMoveSelectionCursor", 1)  # Dragon Rage KOs the 40-HP lead.
    observer.wait(lambda: observer.prompt() or (observer.action_menu() and observer.indexes()[1] == 1),
        "replacement prompt or next turn", advance=True)
    assert observer.prompt() == expect_prompt, ("offer eligibility", expect_prompt, observer.indexes(), observer.mons())
    observer.reset_observation()


def option_row(game):
    # Only the visible value cell. Cycling back to Set must erase longer names.
    path = game.tmp / "option.ppm"
    game.frames(2)
    game.command(f"shot {path}")
    pixels = path.read_bytes().split(b"\n", 3)[3]
    return b"".join(pixels[(y * 240 + 120) * 3:(y * 240 + 216) * 3] for y in range(72, 88))


def select_and_reload_style(game, observer, source, target):
    order = ("set", "switch", "predict") if source else ("shift", "set")
    style_mask = STYLE_BITS["set"] | STYLE_BITS["predict"]

    def options_word():
        return int.from_bytes(game.read(game.u32(game.sym["gSaveBlock2Ptr"]) + 0x14, 2), "little")

    def open_style_row():
        observer.wait(lambda: task_active(game, "Task_OptionMenuProcessInput"), "options input")
        observer.key(DOWN)
        observer.key(DOWN)

    open_style_row()
    original = options_word()
    assert original & style_mask == STYLE_BITS[order[0]], ("fresh battle style", original, order)
    rows = {}
    for style in order:
        rows[style] = option_row(game)
        observer.key(RIGHT)
    assert len(set(rows.values())) == len(order), "battle style labels look identical"
    for style in (order[0], *reversed(order[1:])):
        assert option_row(game) == rows[style], ("option redraw or reverse order", style)
        observer.key(LEFT)
    for _ in range(order.index(target)):
        observer.key(RIGHT)
    assert option_row(game) == rows[target]
    game.screenshot("options-" + target)
    observer.key(B)
    observer.wait(lambda: game.u32(game.sym["gShiftSwitchSaveResult"]), "write options to Flash")
    assert game.u32(game.sym["gShiftSwitchSaveResult"]) == 1
    expected = (original & ~style_mask) | STYLE_BITS[target]
    assert options_word() == expected, ("saved options word", target, options_word(), expected)
    game.command("reset")
    open_style_row()
    assert options_word() == expected and option_row(game) == rows[target], ("reloaded option", target)
    observer.key(B)
    return dict(default=order[0], selected=target, saved_options=expected, flash_reload=True)


def check_prompt_text(game, style):
    raw = game.read(game.sym["gDisplayedStringBattle"], 256).split(b"\xff", 1)[0]
    shown = " ".join("".join(ENGLISH.get(byte, "�") for byte in raw).split())
    announcement = {"predict": "switch Pokémon", "switch": "use Ember", "shift": "send out Ember"}[style]
    expected = f"YOUNGSTER Joey is about to {announcement}. Will you switch your Pokémon?"
    assert shown == expected, ("switch disclosure", shown, expected)
    return shown


def run_case(driver, output, profile, name, definitions):
    source = profile == "source"
    code = (ROOT / "tools/polished_port/tests/shift_switch_fixture.c").read_text()
    for field, value in definitions.items():
        code = code.replace(f"#define SHIFT_PROBE_{field} 0", f"#define SHIFT_PROBE_{field} {value}")
    with tempfile.TemporaryDirectory(prefix="polished-shift-" + name + "-") as directory:
        directory = Path(directory)
        elf = compile_shift_probe(directory, code, profile)
        game = Game(driver, elf, elf, output, directory)
        checks = []
        try:
            observer = Observer(game, source)
            style = {"set-mode": "set", "predict-mode": "predict"}.get(name, "switch" if source else "shift")
            options = select_and_reload_style(game, observer, source, style) if definitions.get("OPTIONS") else None
            expected_prompt = not definitions.get("RESERVE", 0) and not definitions.get("SET_MODE", 0)
            prepare(game, observer, expected_prompt)
            assert game.read(game.sym["gBattleScripting"] + observer.layout["battle_style"], 1)[0] == STYLE_IDS[style]
            if options:
                observer.setup["options"] = options
            if not expected_prompt:
                assert observer.indexes() == (0, 1)
                assert observer.lead_status() != 0
                game.screenshot("shift-" + name + "-no-prompt")
                return [dict(case=name, offered=False, indexes=observer.indexes(), setup=observer.setup)]
            trap = bool(definitions.get("TRAP", 0))
            if trap:
                assert observer.mons()[0]["trapped"] == source, ("faint trap before prompt", observer.mons())
            observer.setup["prompt"] = check_prompt_text(game, style)
            game.screenshot("shift-" + name + "-prompt")
            game.snapshot("prompt")
            cases = ("accept",) if trap else ("accept", "decline", "cancel")
            for choice in cases:
                game.restore("prompt")
                observer.reset_observation()
                observer.key(B if choice == "decline" else A)
                if choice == "accept":
                    observer.choose_reserve()
                    observer.assert_entries(source, trap)
                elif choice == "decline":
                    observer.wait(observer.action_menu, "declined switch reaches next turn", advance=True)
                    assert observer.indexes() == (0, 1)
                    assert observer.lead_status() != 0
                else:
                    observer.wait(observer.party_menu, "party menu before cancel")
                    game.screenshot("shift-" + name + "-cancel-menu")
                    observer.key(B)
                    observer.wait(lambda: observer.prompt() or observer.action_menu(), "party cancellation continuation", advance=True)
                    assert observer.prompt() == source, ("source cancel must reoffer", source, observer.indexes())
                    if source:
                        assert check_prompt_text(game, style) == observer.setup["prompt"]
                        game.screenshot("shift-" + name + "-reoffered")
                        observer.key(A)
                        observer.choose_reserve()
                        observer.assert_entries(source, trap)
                    else:
                        assert observer.indexes() == (0, 1)
                        assert observer.lead_status() != 0
                game.screenshot("shift-" + name + "-" + choice)
                checks.append(dict(case=name, choice=choice, indexes=observer.indexes(),
                    events=list(observer.events), lead_status=observer.lead_status(), setup=observer.setup))
            return checks
        except Exception:
            game.screenshot("shift-" + name + "-failure")
            raise
        finally:
            game.close()


def run_wild_case(driver, output, profile, name, definitions):
    source = profile == "source"
    code = (ROOT / "tools/polished_port/tests/shift_switch_fixture.c").read_text()
    for field, value in {"WILD_FAINT": 1, **definitions}.items():
        code = code.replace(f"#define SHIFT_PROBE_{field} 0", f"#define SHIFT_PROBE_{field} {value}")
    with tempfile.TemporaryDirectory(prefix="polished-faint-" + name + "-") as directory:
        directory = Path(directory)
        elf = compile_shift_probe(directory, code, profile)
        game = Game(driver, elf, elf, output, directory)
        checks = []
        try:
            observer = Observer(game, source)
            observer.wait(observer.action_menu, "initial wild action menu", advance=True)
            observer.assert_initial_setup(wild=True)
            trap = bool(definitions.get("TRAP", 0))
            if trap:
                choose_cell(game, "gActionSelectionCursor", 0)
                observer.wait(lambda: controller(game, "HandleInputChooseMove"), "wild trap move menu")
                choose_cell(game, "gMoveSelectionCursor", 0)  # Splash while the wild foe uses Mean Look.
                observer.wait(observer.action_menu, "wild Mean Look turn completes")
                assert observer.mons()[0]["trapped"], "the live setup never established Mean Look"
            choose_cell(game, "gActionSelectionCursor", 0)
            observer.wait(lambda: controller(game, "HandleInputChooseMove"), "wild self-KO move menu")
            choose_cell(game, "gMoveSelectionCursor", 1)
            observer.wait(lambda: observer.prompt() or observer.finished(), "wild faint prompt or final loss", advance=True)
            assert observer.lead_hp() == 0
            assert observer.last_faint_state is not None, "the live player faint was never observed"
            foe_hp = observer.last_faint_state[1]["hp"]
            assert observer.last_faint_state[0]["hp"] == 0
            assert 0 < foe_hp < observer.setup["initial_mons"][1]["hp"], (
                "live Explosion must damage but not faint the foe", observer.last_faint_state)
            if definitions.get("RESERVE", 0):
                assert observer.finished()
                assert not observer.prompt()
                assert observer.outcome() == observer.layout["outcome_lost"]
                game.screenshot("wild-faint-" + name + "-loss")
                return [dict(case=name, offered=False, outcome=observer.outcome(), lead_hp=observer.lead_hp(),
                    pre_cleanup_opponent_battle_hp=foe_hp,
                    post_cleanup_opponent_party_hp=observer.party_hp(observer.layout["opponent_trainer"]),
                    last_faint_state=observer.last_faint_state, setup=observer.setup)]
            assert observer.prompt() and observer.outcome() == 0
            assert observer.mons()[1]["hp"] == observer.party_hp(observer.layout["opponent_trainer"]) == foe_hp
            observer.assert_no_gimmicks()
            if trap:
                assert observer.mons()[0]["trapped"] == source, ("fainted victim trap lifetime", observer.mons())
            game.screenshot("wild-faint-" + name + "-prompt")
            game.snapshot("wild-prompt")
            for choice in ("accept", "decline"):
                game.restore("wild-prompt")
                observer.reset_observation()
                observer.key(A if choice == "accept" else B)
                needs_replacement = choice == "accept" or (trap and source)
                if needs_replacement:
                    observer.wait(observer.party_menu, "wild mandatory replacement", advance=True)
                    assert observer.outcome() == 0
                    if choice == "accept":
                        # ForcePickSwitchMonInBattle cannot be cancelled. This
                        # uses the real party menu's B handler, not a stub.
                        observer.key(B)
                        for _ in range(40):
                            observer.tick()
                        assert observer.party_menu(), "mandatory replacement accepted cancellation"
                        assert observer.indexes()[0] == 0 and observer.lead_hp() == 0
                    observer.choose_reserve(expected_indexes=(1, 0))
                    assert observer.outcome() == 0 and observer.lead_hp() == 0
                    assert observer.mons()[0]["species"] == observer.layout["arcanine"]
                    assert not observer.mons()[0]["trapped"]
                    assert observer.mons()[1]["attack"] == 5
                    if trap and source and choice == "decline":
                        # The attempted escape failed before the real forced
                        # selector; the source trap then ends on replacement.
                        assert observer.indexes() == (1, 0)
                else:
                    observer.wait(observer.finished, "declining wild replacement escapes", advance=True)
                    assert observer.outcome() == observer.layout["outcome_ran"]
                    assert observer.indexes()[0] == 0 and observer.lead_hp() == 0
                game.screenshot("wild-faint-" + name + "-" + choice)
                checks.append(dict(case=name, choice=choice, source_trap=trap and source,
                    indexes=observer.indexes(), outcome=observer.outcome(), lead_hp=observer.lead_hp(),
                    mandatory_replacement=needs_replacement, events=list(observer.events),
                    opponent_party_hp=observer.party_hp(observer.layout["opponent_trainer"]),
                    last_faint_state=observer.last_faint_state, setup=observer.setup))
            return checks
        except Exception:
            game.screenshot("wild-faint-" + name + "-failure")
            raise
        finally:
            game.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--profile", choices=("source", "native"), required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    checks = []
    trainer_cases = [("ordinary", {"OPTIONS": 1}), ("fainted-trapper", {"TRAP": 1}),
                     ("no-reserve", {"RESERVE": 1}), ("fainted-reserve", {"RESERVE": 2}),
                     ("set-mode", {"SET_MODE": 1, "OPTIONS": 1})]
    if args.profile == "source":
        trainer_cases.append(("predict-mode", {"OPTIONS": 1}))
    for name, definitions in trainer_cases:
        print(f"Checking {args.profile} trainer replacement: {name}", flush=True)
        checks.extend(dict(check, context="trainer_shift") for check in
            run_case(args.driver.resolve(), args.output.resolve(), args.profile, name, definitions))
    for name, definitions in (("ordinary", {}), ("trapped-victim", {"TRAP": 1}),
                              ("no-reserve", {"RESERVE": 1}), ("fainted-reserve", {"RESERVE": 2})):
        print(f"Checking {args.profile} wild faint prompt: {name}", flush=True)
        checks.extend(dict(check, context="wild_faint") for check in
            run_wild_case(args.driver.resolve(), args.output.resolve(), args.profile, name, definitions))
    cases = len(trainer_cases) + 4
    assert len(checks) == (16 if args.profile == "source" else 13)
    assert len({(check["context"], check["case"]) for check in checks}) == cases
    selected = PROFILES[args.profile]
    report = dict(fixture_only=True, source_commit=PIN, profile=args.profile, fixture_source_rules=selected["demo"],
        setup_after_init="read-only assertions; observer supplies real input only", cases=cases, branches=len(checks), checks=checks,
        production_rom=selected["stem"] + ".gba",
        production_rom_sha256=hashlib.sha256((ROOT / (selected["stem"] + ".gba")).read_bytes()).hexdigest(),
        production_map_sha256=hashlib.sha256((ROOT / (selected["stem"] + ".map")).read_bytes()).hexdigest(),
        source_contract="DefaultOptions, Options_BattleStyle, OfferSwitch, AskUseNextPokemon and HandleBetweenTurnEffects")
    (args.output / "shift-switch-validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: {args.profile} battle styles through Flash reload, trainer disclosure and wild faint prompts "
          f"({cases} cases, {len(checks)} branches)", flush=True)


if __name__ == "__main__":
    main()
