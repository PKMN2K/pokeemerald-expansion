"""Exercise the pinned Smart AI field and recovery handlers in the Johto ROM."""
import re
import struct

from audit_moves_machines import ROOT, compile_probe
from build_battle_suite import authenticated_sources, source
from test_johto_demo_emulator import A, B, DOWN, LEFT, RIGHT, UP, Game
from test_new_bark_battle_emulator import choose_cell, controller

WEATHER = {'clear': 'B_WEATHER_NONE', 'rain': 'B_WEATHER_RAIN_NORMAL',
           'sun': 'B_WEATHER_SUN_NORMAL', 'sand': 'B_WEATHER_SANDSTORM', 'hail': 'B_WEATHER_HAIL'}
SETTERS = {'RAIN_DANCE': 'rain', 'SUNNY_DAY': 'sun', 'SANDSTORM': 'sand', 'HAIL': 'hail'}
RECOVERY = {'RECOVER', 'REST', 'FRESH_SNACK', 'ROOST', 'HEALINGLIGHT'}


def cases(draws):
    asm = authenticated_sources()['engine/battle/ai/scoring.asm']['text']
    effects = set(re.findall(r'^\s*dbw EFFECT_(\w+),\s+AI_Smart_(?:Heal|Roost|HealingLight|LightScreen|Reflect|Safeguard|Thunder|RainDance|SunnyDay|Sandstorm|Hail|Toxic|LeechSeed)$', asm, re.M))
    files = source.sources()
    mapped = source.move_targets(files)
    moves = set()
    for line in source.default_lines(files['data/moves/moves.asm']):
        record = re.match(r'move (\w+),\s*EFFECT_(\w+),', line)
        if record and record[2] in effects:
            moves.add(record[1])
    assert moves == set(SETTERS) | RECOVERY | {'REFLECT', 'LIGHT_SCREEN', 'SAFEGUARD', 'THUNDER', 'TOXIC', 'LEECH_SEED'}

    def row(slots, hp, maximum, target, weather, effective, mask, seed):
        stream = draws(seed)
        deltas = []
        for slot, move in enumerate(slots):
            delta = 0
            if mask & (1 << slot):
                pass
            elif move in RECOVERY:
                third = max(1, maximum // 3)
                delta = 2 if hp <= third else 1 if hp < maximum // 2 else 0 if hp < 2 * third else -2
                if move == 'HEALINGLIGHT' and effective != 'clear':
                    delta += 1 if effective == 'sun' else -1
            elif move in SETTERS:
                if SETTERS[move] == weather:
                    delta = -10
                else:
                    delta = 1 + int(weather != 'clear') - 2 * int(2 * hp <= maximum)
            elif move == 'THUNDER' and weather == 'sun':
                delta = -int(next(stream) & 255 >= 25)
            elif move in ('LIGHT_SCREEN', 'REFLECT') and hp < maximum:
                delta = -int(next(stream) & 255 >= 20)
            elif move == 'SAFEGUARD' and target <= 50:
                delta = -int(next(stream) & 255 >= 50)
            elif move in ('TOXIC', 'LEECH_SEED') and target <= 50:
                delta = -1
            deltas.append(str(delta))
        return ('{{' + ', '.join(mapped[m] for m in slots) + '}, '
                + f'{hp}, {maximum}, {target}, {seed}, {next(stream)}, {WEATHER[weather]}, {WEATHER[effective]}, {mask}, '
                + '{' + ', '.join(deltas) + '}},')

    for maximum in (1, 95, 100, 251, 714):
        third = max(1, maximum // 3)
        for hp in sorted({min(maximum, max(1, n + d)) for n in (third, maximum // 2, 2 * third, maximum) for d in (-1, 0, 1)}):
            for weather in WEATHER:
                yield row(('RECOVER', 'REST', 'ROOST', 'FRESH_SNACK'), hp, maximum, 50, weather, weather, 0, 1)
                for effective in ((weather,) if weather == 'clear' else (weather, 'clear')):
                    yield row(('HEALINGLIGHT', 'TOXIC', 'LEECH_SEED', 'TACKLE'), hp, maximum, 50, weather, effective, 0, 1)
    for weather in WEATHER:
        for hp in (49, 50, 51):
            yield row(tuple(SETTERS), hp, 100, 100, weather, 'clear', 0, 1)
    # Exercise both sides of each random-byte threshold and the next draw with holes.
    seeds = {byte: next(seed for seed in range(65536) if next(draws(seed)) & 255 == byte)
             for byte in (19, 20, 24, 25, 49, 50)}
    for seed in seeds.values():
        for hp in (99, 100):
            for target in (49, 50, 51):
                for mask in (0, 1, 3, 7, 15):
                    yield row(('THUNDER', 'REFLECT', 'LIGHT_SCREEN', 'SAFEGUARD'), hp, 100, target, 'sun', 'clear', mask, seed)
                yield row(('TOXIC', 'LEECH_SEED', 'THUNDER', 'SAFEGUARD'), hp, 100, target, 'clear', 'clear', 0, seed)


def replay(driver, directory, output, draws):
    rows = list(cases(draws))
    code = (ROOT/'tools/polished_port/tests/smart_field_ai_fixture.c').read_text()
    elf = compile_probe(ROOT, directory, code.replace('/* SOURCE CASES */', '\n'.join(rows)))
    result = {}
    for name, key in (('low', A), ('full', B), ('low_control', UP), ('full_control', DOWN),
                      ('sun', LEFT), ('sun_umbrella', RIGHT)):
        game = Game(driver, elf, elf, output, directory)
        try:
            game.frames(160)
            audit = struct.unpack('<3I', game.read(game.sym['gSmartAudit'], 12))
            assert audit == (len(rows), 0, 0), audit
            offset, recover, rain, light = struct.unpack('<4I', game.read(game.sym['gSmartLayout'], 16))
            names = {recover: 'Recover', rain: 'Rain Dance', light: 'HealingLight'}
            game.press(key)
            for symbol in ('HandleInputChooseAction', 'HandleInputChooseMove'):
                for _ in range(500):
                    if controller(game, symbol):
                        break
                    game.press(A, 6)
                else:
                    raise AssertionError((name, symbol))
                choose_cell(game, 'gActionSelectionCursor' if symbol.endswith('Action') else 'gMoveSelectionCursor', 0)
            for _ in range(500):
                move = int.from_bytes(game.read(game.sym['gLastMoves'] + 2, 2), 'little')
                if move in names:
                    break
                game.press(A, 6)
            else:
                raise AssertionError((name, 'trainer failed to execute a move', move))
            scores = struct.unpack('<2i', game.read(game.u32(game.sym['gAiBattleData']) + offset, 8))
            result[name] = dict(move=names[move], scores=scores)
        finally:
            game.close()
    for name, delta in (('low', (2, -1)), ('full', (-2, 1))):
        control = result[name + '_control']['scores']
        assert tuple(a - b for a, b in zip(result[name]['scores'], control)) == delta, result
    assert result['low']['move'] == 'Recover' and result['full']['move'] == 'Rain Dance', result
    assert result['sun'] == dict(move='HealingLight', scores=(103, 100)), result
    assert result['sun_umbrella'] == dict(move='HealingLight', scores=(102, 100)), result
    return dict(cases=len(rows), battles=result)
