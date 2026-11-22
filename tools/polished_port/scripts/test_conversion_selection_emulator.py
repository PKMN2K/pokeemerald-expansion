#!/usr/bin/env python3
"""Select a zero-PP Conversion type using the production player controller."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

from audit_moves_machines import compile_probe
from test_johto_demo_emulator import Game, ROOT, A, DOWN
from test_new_bark_battle_emulator import controller, choose_cell


def run(driver, output):
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='polished-conversion-controller-') as tmp:
        directory = Path(tmp)
        elf = compile_probe(ROOT, directory, (ROOT/'tools/polished_port/tests/conversion_selection_fixture.c').read_text())
        game = Game(driver, elf, elf, output, directory)
        try:
            layout = struct.unpack('<5I', game.read(game.sym['gConversionProbeLayout'], 20))
            def wait(predicate, label, advance=True):
                for frame in range(1800):
                    if predicate(): return
                    game.frames(1, A if advance and frame % 24 == 0 else 0)
                raise AssertionError(label)
            def selecting():
                battle = game.u32(game.sym['gBattleStruct'])
                return battle and game.read(battle + layout[0], 1)[0] == 2 and controller(game, 'HandleInputChooseMove')
            wait(lambda: controller(game, 'HandleInputChooseAction'), 'action menu')
            choose_cell(game, 'gActionSelectionCursor', 0)
            wait(lambda: controller(game, 'HandleInputChooseMove'), 'move menu', False)
            game.screenshot('initial-move-menu')
            initial_pp = game.read(game.sym['gBattleMons'] + layout[3], 4)
            choose_cell(game, 'gMoveSelectionCursor', 0)
            wait(selecting, 'Conversion type selection')
            game.screenshot('conversion-type-menu')
            # Current Normal type is invalid: retry rather than spending another PP.
            game.press(A, 30)
            wait(selecting, 'invalid type retry')
            game.press(DOWN, 6)
            assert game.read(game.sym['gMoveSelectionCursor'], 1)[0] == 2
            game.screenshot('conversion-zero-pp-water')
            game.press(A, 30)
            wait(lambda: game.read(game.sym['gBattleMons'] + layout[2], 1)[0] == layout[4], 'Water type applied')
            pp = game.read(game.sym['gBattleMons'] + layout[3], 4)
            assert pp == bytes([initial_pp[0] - 1, *initial_pp[1:]]) and pp[2] == 0, (initial_pp, pp)
            game.screenshot('conversion-water-applied')
        except Exception:
            game.screenshot('failure')
            raise
        finally:
            game.close()
    report = dict(fixture_only=True, production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
        result='real player controller retries an invalid type, selects a zero-PP Water move, changes type and spends only one Conversion PP')
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: production Conversion controller, retry and zero-PP selection', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    run(args.driver.resolve(), args.output.resolve())
