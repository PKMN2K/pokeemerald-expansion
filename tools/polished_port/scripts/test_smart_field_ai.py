"""Exercise the pinned Smart AI trainer move scores in the Johto ROM."""
from itertools import islice, product
from typing import NamedTuple
import re
import struct

import smart_ai_tactics_cases as tactics_cases

from audit_moves_machines import ROOT, compile_probe
from build_battle_suite import authenticated_sources, source
from test_johto_demo_emulator import A, B, DOWN, LEFT, RIGHT, START, UP, Game
from test_new_bark_battle_emulator import choose_cell, controller

WEATHER = {'clear': 'B_WEATHER_NONE', 'rain': 'B_WEATHER_RAIN_NORMAL',
           'sun': 'B_WEATHER_SUN_NORMAL', 'sand': 'B_WEATHER_SANDSTORM', 'hail': 'B_WEATHER_HAIL'}
SETTERS = {'RAIN_DANCE': 'rain', 'SUNNY_DAY': 'sun', 'SANDSTORM': 'sand', 'HAIL': 'hail'}
RECOVERY = {'RECOVER', 'REST', 'FRESH_SNACK', 'ROOST', 'HEALINGLIGHT'}
SPEED_CONTROL = {'AGILITY', 'STRING_SHOT', 'SCARY_FACE'}
PARALYZE = {'GLARE', 'THUNDER_WAVE', 'STUN_SPORE'}
HP_MOVES = {'SUBSTITUTE', 'DESTINY_BOND', 'BELLY_DRUM', 'PAIN_SPLIT', 'EXPLOSION', 'HYPER_BEAM', 'GIGA_IMPACT', 'PURSUIT'}
SELECT = 4

R_BUTTON = 256
L_BUTTON = 512
STATUS_HANDLERS = {
    'AlwaysHit': {'AERIAL_ACE', 'SWIFT', 'FEINT_ATTACK', 'AURA_SPHERE', 'DISARM_VOICE'},
    'Confuse': {'SUPERSONIC', 'CONFUSE_RAY', 'SWEET_KISS'},
    'Attract': {'ATTRACT'}, 'Swagger': {'SWAGGER'}, 'Stomp': {'BODY_SLAM', 'STOMP'},
    'Defrost': {'FLARE_BLITZ', 'SACRED_FIRE'}, 'SpDefenseUp2': {'AMNESIA'}, 'SleepTalk': {'SLEEP_TALK'},
}
STATUS_MOVES = set().union(*STATUS_HANDLERS.values())


class StatusState(NamedTuple):
    accuracy: int = 0
    evasion: int = 0
    sp_defense: int = 0
    minimized: bool = False
    early_bird: bool = False
    status: int | str = 0  # Integer sleep counter, or a non-sleep GBA status constant.


def status_score(move, hp, maximum, target, target_max, first, state, stream):
    if move in STATUS_HANDLERS['AlwaysHit']:
        eligible = state.accuracy <= -3 or state.evasion >= 3
        return 2 if eligible and next(stream) & 255 >= 50 else 0
    if move in STATUS_HANDLERS['Confuse']:
        if target > target_max // 2:
            return 0
        return -int(next(stream) & 255 >= 25) - int(target <= target_max // 4)
    if move in ('SWAGGER', 'ATTRACT'):
        byte = next(stream) & 255
        return int(byte < 200) if first else -int(byte >= 50)
    if move in STATUS_HANDLERS['Stomp']:
        return int(state.minimized and next(stream) & 255 >= 50)
    if move in STATUS_HANDLERS['Defrost']:
        return 5 if state.status == 'STATUS1_FREEZE' else 0
    if move == 'AMNESIA':
        if hp <= maximum // 2 or state.sp_defense >= 4:
            return -1
        return 2 if state.sp_defense < 2 and next(stream) & 255 >= 50 else 0
    assert move == 'SLEEP_TALK'
    sleep = state.status if isinstance(state.status, int) else 0
    return 10 if sleep >= 2 + state.early_bird else -10


def status_cases(row, draws):
    def emit(slots, state, *, hp=100, target=100, first=False, mask=0, seed=7):
        return row(slots, hp, 101, target, 'clear', 'clear', mask, seed, first=first, target_max=101, state=state)

    sure_hit = [('AERIAL_ACE', 'SWIFT', 'FEINT_ATTACK', 'DISARM_VOICE'),
                ('AURA_SPHERE', 'BODY_SLAM', 'STOMP', 'AMNESIA')]
    for slots, acc, evasion, minimized, mask in product(sure_hit, (-3, -2, 0), (0, 2, 3), (False, True), (0, 1, 5, 15)):
        yield emit(slots, StatusState(accuracy=acc, evasion=evasion, minimized=minimized), mask=mask)
    for slots, first, target, mask in product(
            [('SUPERSONIC', 'CONFUSE_RAY', 'SWEET_KISS', 'ATTRACT'), ('SWAGGER', 'ATTRACT', 'SWEET_KISS', 'REFLECT')],
            (False, True), (25, 26, 50, 51), (0, 2, 9, 15)):
        yield emit(slots, StatusState(), first=first, target=target, mask=mask)
    for hp, sp_defense in product((50, 51, 101), (-6, 1, 2, 3, 4, 6)):
        yield emit(('AMNESIA', 'AMNESIA', 'CONFUSE_RAY', 'ATTRACT'), StatusState(sp_defense=sp_defense), hp=hp)
    for status, early_bird, mask in product(
            (0, 1, 2, 3, 7, 'STATUS1_FREEZE', 'STATUS1_FROSTBITE', 'STATUS1_BURN'), (False, True), (0, 1, 6)):
        yield emit(('SLEEP_TALK', 'SACRED_FIRE', 'FLARE_BLITZ', 'STOMP'),
                   StatusState(status=status, early_bird=early_bird), mask=mask)
    # First-slot checks pin both edges without consuming a draw in an earlier slot.
    for move, cutoff, first in [('SWIFT', 50, False), ('CONFUSE_RAY', 25, False), ('BODY_SLAM', 50, False),
                                ('ATTRACT', 200, True), ('SWAGGER', 50, False), ('AMNESIA', 50, False)]:
        for byte in (cutoff - 1, cutoff):
            seed = next(n for n in range(65536) if next(draws(n)) & 255 == byte)
            yield emit((move, 'SLEEP_TALK', 'STOMP', 'ATTRACT'), StatusState(accuracy=-3, minimized=True),
                       target=25, first=first, seed=seed)


def hp_score(move, hp, maximum, target, target_max, attack_stage, last_against_reserve, stream):
    half = 2 * hp <= maximum
    quarter = 4 * hp <= maximum
    if move == 'SUBSTITUTE':
        return -10 * half
    if move == 'DESTINY_BOND':
        return -int(not quarter)
    if move == 'PAIN_SPLIT':
        return -int(target < 2 * hp)
    if move == 'BELLY_DRUM':
        if attack_stage >= 3:
            return -5
        return -(1 + 5 * half) if hp < maximum else 0
    if move == 'EXPLOSION':
        if last_against_reserve or not half:
            return -3
        return -3 if not quarter and next(stream) & 255 >= 20 else 0
    if move == 'PURSUIT':
        if 4 * target <= target_max:
            return 2 * int(next(stream) & 255 >= 128)
        return -int(next(stream) & 255 >= 50)
    assert move in ('HYPER_BEAM', 'GIGA_IMPACT')
    if not half and next(stream) & 255 >= 40:
        return -1 - int(next(stream) & 255 >= 128)
    return int(quarter and next(stream) & 255 >= 128)


def hp_cases(row, draws):
    fixed = ('SUBSTITUTE', 'DESTINY_BOND', 'BELLY_DRUM', 'PAIN_SPLIT')
    for maximum in (100, 301, 714):
        for hp in (1, maximum // 4, maximum // 4 + 1, maximum // 2, maximum // 2 + 1, maximum - 1, maximum):
            for diff, stage, mask in product((-1, 0, 1), (-6, 2, 3, 6), (0, 6)):
                yield row(fixed, hp, maximum, 2 * hp + diff, 'clear', 'clear', mask, 1,
                          target_max=2 * maximum + 1, attack_stage=stage)

    mixed = ('EXPLOSION', 'HYPER_BEAM', 'GIGA_IMPACT', 'PURSUIT')
    for hp, target, last, mask in product((25, 26, 50, 51), (75, 76), (False, True), (0, 1, 5, 15)):
        for slots in (mixed, mixed[::-1]):
            yield row(slots, hp, 100, target, 'clear', 'clear', mask, 7, target_max=301, last_against_reserve=last)

    # Force each random branch on both sides of its threshold, including the
    # conditional second penalty for recharge moves at high HP.
    trials = [('EXPLOSION', 50, 100, 20, False),
              ('PURSUIT', 100, 25, 128, False), ('PURSUIT', 100, 26, 50, False)]
    for move in ('HYPER_BEAM', 'GIGA_IMPACT'):
        trials.extend(((move, 25, 100, 128, False), (move, 51, 100, 40, False),
                       (move, 51, 100, 128, True)))
    for move, hp, target, cutoff, second_draw in trials:
        for byte in (cutoff - 1, cutoff):
            for seed in range(65536):
                stream = draws(seed)
                if second_draw and next(stream) & 255 < 40:
                    continue
                if next(stream) & 255 == byte:
                    break
            else:
                raise AssertionError((move, byte))
            yield row((move, 'REFLECT', 'PURSUIT', 'DESTINY_BOND'), hp, 100, target, 'clear', 'clear', 0, seed)


def cases(draws):
    asm = authenticated_sources()['engine/battle/ai/scoring.asm']['text']
    effects = set(re.findall(r'^\s*dbw EFFECT_(\w+),\s+AI_Smart_(?:Heal|Roost|HealingLight|LightScreen|Reflect|Safeguard|Thunder|RainDance|SunnyDay|Sandstorm|Hail|Toxic|LeechSeed|SpeedControl|SpeedDownHit|TrickRoom|Paralyze|Substitute|DestinyBond|BellyDrum|PainSplit|Explosion|HyperBeam|Pursuit)$', asm, re.M))
    smart_table = dict(re.findall(r'^\s*dbw EFFECT_(\w+),\s+AI_Smart_(\w+)$', asm, re.M))
    handlers = STATUS_HANDLERS | tactics_cases.HANDLERS
    status_moves = {handler: set() for handler in handlers}
    files = source.sources()
    mapped = source.move_targets(files)
    moves = set()
    for line in source.default_lines(files['data/moves/moves.asm']):
        record = re.match(r'move (\w+),\s*EFFECT_(\w+),', line)
        if record and smart_table.get(record[2]) in handlers:
            status_moves[smart_table[record[2]]].add(record[1])
        if record and record[2] in effects:
            moves.add(record[1])
    assert status_moves == handlers
    assert moves == (set(SETTERS) | RECOVERY | SPEED_CONTROL | PARALYZE | HP_MOVES |
                     {'REFLECT', 'LIGHT_SCREEN', 'SAFEGUARD', 'THUNDER', 'TOXIC', 'LEECH_SEED',
                      'TRICK_ROOM', 'ICY_WIND', 'BUBBLE_BEAM', 'BULLDOZE'})

    def row(slots, hp, maximum, target, weather, effective, mask, seed, speeds=(0, 0), room=False, first=False,
            target_max=100, attack_stage=0, last_against_reserve=False, state=StatusState(), tactics=None):
        tactics = tactics or {}
        usable = {move for i, move in enumerate(slots) if not mask & (1 << i)}
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
            elif move == 'SAFEGUARD' and 2 * target <= target_max:
                delta = -int(next(stream) & 255 >= 50)
            elif move in ('TOXIC', 'LEECH_SEED') and 2 * target <= target_max:
                delta = -1
            elif move in SPEED_CONTROL | PARALYZE | {'ICY_WIND', 'TRICK_ROOM'}:
                low_user = 4 * hp <= maximum
                low_target = 4 * target <= target_max
                compares = not ((move == 'ICY_WIND' and (low_user or not first)) or
                                (move in PARALYZE and low_target))
                if compares and speeds[0] == speeds[1]:
                    next(stream)
                faster = speeds[1] > speeds[0] if room else speeds[0] > speeds[1]
                if move in SPEED_CONTROL:
                    delta = -10 if faster else 0
                elif move == 'TRICK_ROOM':
                    delta = -10 if faster else 0 if room else 3
                elif move == 'ICY_WIND':
                    if compares and not faster:
                        delta = 2 * int(next(stream) & 255 >= 30)
                elif low_target:
                    delta = -int(next(stream) & 255 >= 128)
                elif not faster and not low_user:
                    delta = 2 * int(next(stream) & 255 >= 50)
            elif move in HP_MOVES:
                delta = hp_score(move, hp, maximum, target, target_max, attack_stage, last_against_reserve, stream)
            elif move in STATUS_MOVES:
                delta = status_score(move, hp, maximum, target, target_max, first, state, stream)
            elif move in tactics_cases.MOVES:
                delta = tactics_cases.score(move, tactics, usable, hp, maximum, first, speeds, room, state, stream)
            deltas.append(str(delta))
        inputs = ', '.join(f'.{key} = {value}' for key, value in tactics.items()) or '0'
        return ('{{' + ', '.join(mapped[m] for m in slots) + '}, '
                + f'{hp}, {maximum}, {target}, {seed}, {next(stream)}, {WEATHER[weather]}, {WEATHER[effective]}, {mask}, '
                + '{' + ', '.join(deltas) + '}, ' + f'{speeds[0]}, {speeds[1]}, {int(room)}, {int(first)}, '
                + f'{target_max}, DEFAULT_STAT_STAGE + ({attack_stage}), {int(last_against_reserve)}, '
                + f'{state.accuracy}, {state.evasion}, {state.sp_defense}, {int(state.minimized)}, {int(state.early_bird)}, {state.status}, ' + '{' + inputs + '}},')

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

    # Mixed field and speed handlers must consume draws in usable slot order.
    groups = (('AGILITY', 'SCARY_FACE', 'STRING_SHOT', 'TRICK_ROOM'),
              ('THUNDER_WAVE', 'REFLECT', 'ICY_WIND', 'GLARE'),
              ('STUN_SPORE', 'ICY_WIND', 'BUBBLE_BEAM', 'BULLDOZE'))
    speeds_to_test = ((80, 120), (120, 80), (100, 100))
    for slots, speeds, room, first, hp, target, mask in product(
            groups, speeds_to_test, (False, True), (False, True), (25, 26), (25, 26), (0, 2, 5, 15)):
        yield row(slots, hp, 101, target, 'clear', 'clear', mask, 7, speeds, room, first)
    # Put the chance-bearing move first and force the byte either side of each cutoff.
    for slots, target, cutoff in ((('ICY_WIND', 'STUN_SPORE', 'TRICK_ROOM', 'AGILITY'), 100, 30),
                                  (('GLARE', 'THUNDER_WAVE', 'ICY_WIND', 'REFLECT'), 26, 50),
                                  (('THUNDER_WAVE', 'TRICK_ROOM', 'REFLECT', 'GLARE'), 25, 128)):
        for speeds in ((80, 120), (100, 100)):
            skip = int(speeds[0] == speeds[1] and target > 25)
            for byte in (cutoff - 1, cutoff):
                seed = next(n for n in range(65536) if next(islice(draws(n), skip, None)) & 255 == byte)
                yield row(slots, 26, 100, target, 'clear', 'clear', 0, seed, speeds, False, True)

    yield from hp_cases(row, draws)
    yield from status_cases(row, draws)
    yield from tactics_cases.cases(row, draws, StatusState)


def replay(driver, directory, output, draws):
    rows = list(cases(draws))
    code = (ROOT/'tools/polished_port/tests/smart_field_ai_fixture.c').read_text()
    elf = compile_probe(ROOT, directory, code.replace('/* SOURCE CASES */', '\n'.join(rows)))
    result = {}
    for name, key in (('low', A), ('full', B), ('low_control', UP), ('full_control', DOWN),
                      ('sun', LEFT), ('sun_umbrella', RIGHT), ('faster', START | A), ('player_scarf', START | B),
                      ('player_reserve', SELECT | A), ('fainted_reserve', SELECT | B),
                      ('egg_reserve', SELECT | LEFT), ('both_reserves', SELECT | RIGHT),
                      ('asleep', R_BUTTON | A), ('early_bird', R_BUTTON | B), ('gas', R_BUTTON | RIGHT),
                      ('frozen', R_BUTTON | LEFT), ('thawed', R_BUTTON | START | LEFT),
                      ('awake_target', L_BUTTON | A), ('sleeping_target', L_BUTTON | B), ('no_dream_pp', L_BUTTON | START | A),
                      ('awake_basic', L_BUTTON | UP), ('sleeping_basic', L_BUTTON | DOWN),
                      ('poisoned_reserve', L_BUTTON | RIGHT), ('fainted_poisoned_reserve', L_BUTTON | LEFT)):
        game = Game(driver, elf, elf, output, directory)
        try:
            game.frames(160)
            audit = struct.unpack('<3I', game.read(game.sym['gSmartAudit'], 12))
            assert audit == (len(rows), 0, 0), audit
            (offset, counter, recover, rain, light, room, splash, explosion, split, sleep_talk, sacred_fire,
             spore, dream, bell, bond) = struct.unpack('<15I', game.read(game.sym['gSmartLayout'], 60))
            names = {recover: 'Recover', rain: 'Rain Dance', light: 'HealingLight', room: 'Trick Room', splash: 'Splash',
                     explosion: 'Explosion', split: 'Pain Split', sleep_talk: 'Sleep Talk', sacred_fire: 'Sacred Fire',
                     spore: 'Spore', dream: 'Dream Eater', bell: 'Heal Bell', bond: 'Destiny Bond'}
            # Capture the announced move before self-KO or Sleep Talk's called move.
            history = game.sym['gLastPrintedMoves' if key & (SELECT | R_BUTTON | L_BUTTON) else 'gLastMoves']
            game.press(key)
            turns = (name, 'room_active') if name == 'player_scarf' else (name,)
            for index, turn in enumerate(turns):
                for symbol in ('HandleInputChooseAction', 'HandleInputChooseMove'):
                    for _ in range(500):
                        if controller(game, symbol):
                            break
                        game.press(A, 6)
                    else:
                        raise AssertionError((turn, symbol))
                    assert game.read(game.sym['gBattleResults'] + counter, 1)[0] == index, turn
                    choose_cell(game, 'gActionSelectionCursor' if symbol.endswith('Action') else 'gMoveSelectionCursor', 0)
                for _ in range(500):
                    move = int.from_bytes(game.read(history + 2, 2), 'little')
                    finished = game.read(game.sym['gBattleResults'] + counter, 1)[0] > index
                    if move in names and (index == 0 or names[move] != result[name]['move'] or finished):
                        break
                    game.press(A, 6)
                else:
                    raise AssertionError((turn, 'trainer failed to execute a move', move))
                scores = struct.unpack('<2i', game.read(game.u32(game.sym['gAiBattleData']) + offset, 8))
                result[turn] = dict(move=names[move], scores=scores)
        finally:
            game.close()
    for name, delta in (('low', (2, -1)), ('full', (-2, 1))):
        control = result[name + '_control']['scores']
        assert tuple(a - b for a, b in zip(result[name]['scores'], control)) == delta, result
    assert result['low']['move'] == 'Recover' and result['full']['move'] == 'Rain Dance', result
    assert result['sun'] == dict(move='HealingLight', scores=(103, 100)), result
    assert result['sun_umbrella'] == dict(move='HealingLight', scores=(102, 100)), result
    assert result['faster'] == dict(move='Splash', scores=(90, 100)), result
    assert result['player_scarf'] == dict(move='Trick Room', scores=(103, 100)), result
    assert result['room_active'] == dict(move='Splash', scores=(90, 100)), result
    assert result['player_reserve'] == dict(move='Pain Split', scores=(97, 99)), result
    for name in ('fainted_reserve', 'egg_reserve', 'both_reserves'):
        assert result[name] == dict(move='Explosion', scores=(100, 99)), result
    for name in ('asleep', 'gas'):
        assert result[name] == dict(move='Sleep Talk', scores=(110, 100)), result
    assert result['early_bird'] == dict(move='Splash', scores=(90, 100)), result
    assert result['frozen'] == dict(move='Sacred Fire', scores=(105, 102)), result
    assert result['thawed'] == dict(move='Recover', scores=(100, 102)), result
    tactics_cases.check_battles(result)
    return dict(cases=len(rows), battles=result)
