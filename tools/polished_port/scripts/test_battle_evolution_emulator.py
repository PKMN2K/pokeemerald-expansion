#!/usr/bin/env python3
"""Press B in the real evolution UI and verify a safe return to the battle.

Uses the compiled native battle-test fixture. Only the test runner's selection
and headless flags are patched, exactly as `make check` does; gameplay memory,
party data, scripts and the evolution implementation are never overwritten.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

from test_johto_demo_emulator import Game, B, ROOT
from test_new_bark_intro_emulator import party


def constants():
    fields = {
        "species": "offsetof(struct BattlePokemon, species)",
        "item": "offsetof(struct BattlePokemon, item)",
        "ability": "offsetof(struct BattlePokemon, ability)",
        "types": "offsetof(struct BattlePokemon, types)",
        "clock": "offsetof(struct MusicPlayerInfo, clock)",
        "poliwhirl": "SPECIES_POLIWHIRL", "politoed": "SPECIES_POLITOED",
        "kings_rock": "ITEM_KINGS_ROCK", "drizzle": "ABILITY_DRIZZLE",
        "swift_swim": "ABILITY_SWIFT_SWIM", "grass": "TYPE_GRASS",
        "rain": "B_WEATHER_RAIN_NORMAL",
    }
    source = ('#include "global.h"\n#include "battle.h"\n#include "m4a.h"\n'
              'const u32 values[] = {' + ','.join(fields.values()) + '};\n')
    result = subprocess.run(["arm-none-eabi-gcc", "-mcpu=arm7tdmi", "-mthumb", "-mabi=apcs-gnu",
        "-DMODERN=1", "-DEMERALD", "-std=gnu17", "-iquote", "include", "-iquote", ".",
        "-S", "-x", "c", "-o", "-", "-"], cwd=ROOT, input=source, text=True, capture_output=True, check=True)
    values = list(map(int, re.findall(r"\.word\s+(\d+)", result.stdout)))
    assert len(values) == len(fields)
    return dict(zip(fields, values))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    c = constants()
    with tempfile.TemporaryDirectory() as tmp_name:
        tmp = Path(tmp_name)
        elf = tmp / "evolution-input-test.elf"
        shutil.copy2(args.elf, elf)
        subprocess.run([str(ROOT / "tools/patchelf/patchelf"), str(elf),
            "gTestRunnerHeadless", r"\x01", "gTestRunnerArgv",
            r"Polished evolution refreshes source types\0"], check=True)
        game = Game(args.driver.resolve(), elf, elf, args.output.resolve(), tmp)
        try:
            for _ in range(2000):
                if game.u32(game.sym["sBattleEvolution"]):
                    break
                game.frames(1)
            else:
                raise AssertionError("Fixture did not reach mid-trainer evolution")
            environment = game.read(game.sym["gBattleEnvironment"], 1)
            music = game.u32(game.sym["gMPlayInfo_BGM"])
            clock = game.u32(game.sym["gMPlayInfo_BGM"] + c["clock"])
            for _ in range(120):
                if game.u32(game.sym["gMain"] + 4) == game.sym["CB2_EvolutionSceneUpdate"] | 1:
                    break
                game.frames(1)
            else:
                raise AssertionError("Evolution UI did not open")
            game.snapshot("before-evolution")
            checks = []
            for cancel in (False, True):
                game.restore("before-evolution")
                for frames in range(5000):
                    callback = game.u32(game.sym["gMain"] + 4)
                    running = game.u32(game.sym["sBattleEvolution"])
                    if callback == game.sym["BattleMainCB2"] | 1 and running == 0:
                        break
                    game.frames(1, B if cancel and callback == game.sym["CB2_EvolutionSceneUpdate"] | 1 else 0)
                else:
                    game.screenshot("evolution-stalled")
                    raise AssertionError(f"Evolution did not resume the battle: cancel={cancel}")
                mon = party(game)[0]
                expected_species = c["poliwhirl"] if cancel else c["politoed"]
                expected_item = c["kings_rock"] if cancel else 0
                assert mon["species"] == expected_species, mon
                assert mon["item"] == expected_item, mon
                def battle_half(field):
                    return struct.unpack("<H", game.read(game.sym["gBattleMons"] + c[field], 2))[0]
                assert battle_half("species") == expected_species
                assert battle_half("item") == expected_item
                assert battle_half("ability") == c["swift_swim" if cancel else "drizzle"]
                assert not game.read(game.sym["gLeveledUpInBattle"], 1)[0] & 1
                assert bool(game.u32(game.sym["gBattleWeather"]) & c["rain"]) == (not cancel)
                assert game.read(game.sym["gBattleEnvironment"], 1) == environment
                assert game.u32(game.sym["gMPlayInfo_BGM"]) == music
                assert game.u32(game.sym["gMPlayInfo_BGM"] + c["clock"]) >= clock
                if not cancel:
                    assert game.read(game.sym["gBattleMons"] + c["types"] + 1, 1)[0] == c["grass"]
                name = "cancelled" if cancel else "completed"
                game.screenshot("evolution-" + name + "-battle")
                checks.append(dict(result=name, frames=frames, party=mon,
                    music_restored=True, level_up_not_deferred=True, ability_refreshed=not cancel))
            (args.output / "evolution-input-validation.json").write_text(json.dumps(checks, indent=2) + "\n")
            print("PASS: real evolution completion/B cancellation, battle return, held item, ability, music and deferred flag")
        finally:
            game.close()


if __name__ == "__main__":
    main()
