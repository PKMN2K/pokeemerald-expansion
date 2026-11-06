#!/usr/bin/env python3
"""Real evolution input, including B cancellation, for an inherited pattern."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

from test_johto_demo_emulator import Game, B, ROOT
from test_new_bark_intro_emulator import party


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp_name:
        tmp = Path(tmp_name)
        elf = tmp / "form-evolution-input.elf"
        shutil.copy2(args.elf, elf)
        # Only select a native fixture and its runner flags. No gameplay data,
        # evolution implementation or emulator RAM is modified by this driver.
        subprocess.run([str(ROOT / "tools/patchelf/patchelf"), str(elf),
            "gTestRunnerHeadless", r"\x01", "gTestRunnerArgv",
            r"Polished patterned Magikarp evolves\0"], check=True)
        game = Game(args.driver.resolve(), elf, elf, args.output.resolve(), tmp)
        try:
            for _ in range(4000):
                if game.u32(game.sym["gMain"] + 4) == game.sym["CB2_EvolutionSceneUpdate"] | 1:
                    break
                game.frames(1)
            else:
                raise AssertionError("Pattern fixture did not reach evolution")
            before = party(game, details=True)[0]
            assert before["species"] == 1602 and before["polished_form"] == 20, before
            game.snapshot("before-pattern-evolution")
            checks = []
            for cancel in (False, True):
                game.restore("before-pattern-evolution")
                for frames in range(5000):
                    callback = game.u32(game.sym["gMain"] + 4)
                    if callback == game.sym["BattleMainCB2"] | 1 and not game.u32(game.sym["sBattleEvolution"]):
                        break
                    game.frames(1, B if cancel and callback == game.sym["CB2_EvolutionSceneUpdate"] | 1 else 0)
                else:
                    raise AssertionError(f"Pattern evolution stalled: cancel={cancel}")
                after = party(game, details=True)[0]
                assert after["species"] == (before["species"] if cancel else 130), after
                assert after["polished_form"] == before["polished_form"] == 20, after
                assert (after["personality"], after["trainer"]) == (before["personality"], before["trainer"])
                result = "cancelled" if cancel else "completed"
                game.screenshot("pattern-evolution-" + result)
                checks.append(dict(result=result, frames=frames, party=after, hidden_pattern_retained=True))
            (args.output / "form-evolution-input-validation.json").write_text(json.dumps(checks, indent=2) + "\n")
            print("PASS: real patterned evolution completion/B cancellation and persistent breeding pattern")
        finally:
            game.close()


if __name__ == "__main__":
    main()
