"""Real-input first Lyra battle, both outcomes, Potion and saved progression."""
from functools import lru_cache
import json
import re
import tempfile
from pathlib import Path

from build_new_bark import source_files
from convert_player_walking import _read_png_2bit_grayscale
import struct

from test_johto_demo_emulator import A, B, UP, DOWN, LEFT, RIGHT
from test_new_bark_emulator import walk, travel, actors, resident_id, save_game, continue_game, player_state
from test_new_bark_intro_emulator import layout, var, party, script_active, text_matches, advance, task_active


@lru_cache
def portrait_data(trainer='lyra1'):
    files = source_files()
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / 'lyra.png'
        path.write_bytes(files['gfx/trainers/'+trainer+'.png'])
        width, height, source = _read_png_2bit_grayscale(path)
    assert (width, height) == (56, 56)
    def pixel(x, y):
        return 3 - source[y - 8][x - 4] if 4 <= x < 60 and 8 <= y < 64 else 0
    tiles = bytes(pixel(x + tx, y + ty) | pixel(x + tx + 1, y + ty) << 4
        for ty in range(0, 64, 8) for tx in range(0, 64, 8) for y in range(8) for x in range(0, 8, 2))
    colors = [tuple(map(int, rgb)) for rgb in re.findall(r'RGB\s+(\d+),\s*(\d+),\s*(\d+)', files['gfx/trainers/'+trainer+'.pal'].decode())]
    palette = struct.pack('<16H', 0x7fff, *(r | g << 5 | b << 10 for r, g, b in colors), *([0] * 13))
    return tiles, palette


def check_portrait(game, label, trainer='lyra1'):
    battle = game.u32(game.sym['gBattleStruct'])
    if not battle:
        return False
    sprite = game.read(battle + layout()['trainer_sprites'] + 1, 1)[0]
    if sprite >= 64:
        return False
    raw = game.read(game.sym['gSprites'] + sprite * 68, 68)
    # Wait for the trainer to finish sliding in, so the evidence includes the
    # visible portrait rather than an early frame behind the intro shutter.
    if not raw[0x3e] & 1 or raw[0x3e] & 4 or struct.unpack_from('<h', raw, 0x24)[0] != 0:
        return False
    attr = struct.unpack_from('<H', raw, 4)[0]
    tiles, palette = portrait_data(trainer)
    offset = (16 + (attr >> 12)) * 32
    if game.read(game.sym['gPlttBufferFaded'] + offset, 32) != palette:
        return False
    assert game.read(0x6010000 + (attr & 0x3ff) * 32, len(tiles)) == tiles
    game.screenshot('battle-' + label + '-portrait')
    return True


def potion_count(game):
    data = layout()
    key = game.u32(game.u32(game.sym['gSaveBlock2Ptr']) + data['encryption_key']) & 0xffff
    start = game.u32(game.sym['gSaveBlock1Ptr']) + data['items']
    slots = struct.iter_unpack('<HH', game.read(start, data['items_count'] * 4))
    return sum(count ^ key for item, count in slots if item == data['potion'])


def controller(game, name):
    return game.u32(game.sym['gBattlerControllerFuncs']) in [value | 1 for value in game.sym[name + '@all']]


def choose_cell(game, symbol, target):
    for _ in range(4):
        current = game.read(game.sym[symbol], 1)[0]
        if current == target:
            game.press(A, 30)
            return
        game.press((RIGHT if target & 1 else LEFT) if (current ^ target) & 1 else (DOWN if target & 2 else UP), 6)
    raise AssertionError((symbol, target, current))


def finish_battle(game, maps, label, lose=False, expected=None):
    lab = maps['PolishedElmsLab']
    lyra = resident_id(lab, 'LOCALID_ELMS_LAB_LYRA')
    assert var(game, 'lab_scene') == 6
    walk(game, lab, (4, 5))
    game.step(DOWN)
    advance(game, lambda: text_matches(game, 'PolishedBattle_ElmsLabLyraChallengeText'))
    assert game.position() == (4, 6)
    game.screenshot('battle-' + label + '-challenge')
    observed = None
    portrait_seen = False
    switches, status_turns = 0, 0
    for frame in range(2500 if lose else 1500):
        if not portrait_seen and observed is None:
            portrait_seen = check_portrait(game, label)
        if task_active(game, 'Task_HandleChooseMonInput'):
            # Real forced or voluntary switch; never keep selecting a fainted
            # lead. This demo has exactly the original Totodile and the gift.
            mons = party(game, details=True)
            healthy = [i for i, mon in enumerate(mons) if mon['hp']]
            assert healthy
            # Party-menu order puts the active battler in the first slot;
            # gBattlerPartyIndexes still refers to the pre-menu battle order.
            target = healthy[0] if len(healthy) == 1 else 1
            selected = game.read(game.sym['gPartyMenu'] + 9, 1)[0]
            if selected != target:
                game.press(RIGHT if target == 1 else LEFT, 12)
            else:
                game.press(A, 30)
        elif controller(game, 'HandleInputChooseAction'):
            # A switching-only battle can end without opening the move menu.
            # Observe the opponent before choosing any action so its result is
            # still checked immediately when the game returns to the field.
            if observed is None:
                observed = party(game, enemy=True, details=True)
                assert len(observed) == 1
                if expected:
                    assert observed[0]['species'] == layout()[expected], observed
                assert observed[0]['level'] == 5 and observed[0]['item'] == 0
                if expected:
                    nickname, moves = {'Chikorita': ('Chicory', ('tackle', 'growl')),
                        'Cyndaquil': ('Cinder', ('tackle', 'leer')),
                        'Totodile': ('Toto', ('scratch', 'leer'))}[expected]
                    encoded = bytes(0xbb + ord(c) - ord('A') if c.isupper() else 0xd5 + ord(c) - ord('a') for c in nickname)
                    assert bytes.fromhex(observed[0]['nickname']).split(b'\xff', 1)[0] == encoded
                    assert observed[0]['moves'] == [*(layout()[move] for move in moves), 0, 0]
                game.screenshot('battle-' + label + '-first-turn')
            if lose and sum(bool(mon['hp']) for mon in party(game, details=True)) > 1:
                # Spend native turns switching while both Pokémon are alive.
                # Burning all of the lead's Leer PP before its partner fainted
                # could leave this test endlessly selecting an exhausted move.
                choose_cell(game, 'gActionSelectionCursor', 2)
                switches += 1
            else:
                choose_cell(game, 'gActionSelectionCursor', 0)
        elif controller(game, 'HandleInputChooseMove'):
            # Once only one Pokémon remains, its untouched status move permits
            # a genuine defeat. Fail immediately with useful diagnostics if a
            # future roster change removes that input, instead of stalling.
            if lose:
                pp = game.read(game.sym['gBattleMons'] + layout()['battle_pp'], 4)
                assert pp[1], ('Loss test exhausted its status move', label, switches,
                               status_turns, party(game, details=True))
            choose_cell(game, 'gMoveSelectionCursor', 1 if lose else 0)
            status_turns += lose
        else:
            game.press(A, 30)
        if observed and game.u32(game.sym['gMain'] + 4) == game.sym['CB2_Overworld'] | 1:
            break
    else:
        raise AssertionError(('Battle did not return', label, switches, status_turns,
                              party(game, details=True)))
    assert observed is not None and portrait_seen
    outcome = game.read(game.sym['gBattleOutcome'], 1)[0]
    assert outcome == (2 if lose else 1), (label, outcome)
    advance(game, lambda: var(game, 'lab_scene') == 5 and not script_active(game))
    assert game.position() == (4, 6) and lyra not in actors(game)
    assert not player_state(game)['frozen']
    healed = party(game, details=True)
    assert all(mon['hp'] == mon['max_hp'] and not mon['status'] for mon in healed)
    game.screenshot('battle-' + label + '-complete')
    return {'portrait_pixels_and_palette_match': portrait_seen, 'opponent': observed[0], 'outcome': outcome,
            'party_after_heal': healed, 'voluntary_switches': switches, 'status_turns': status_turns}


def take_potion(game, maps, x, label):
    lab = maps['PolishedElmsLab']
    before = potion_count(game)
    walk(game, lab, (x, 7))
    game.step(DOWN)
    advance(game, lambda: text_matches(game, 'PolishedBattle_AideText_GiveYouPotions'))
    aide = resident_id(lab, 'LOCALID_ELMS_LAB_AIDE')
    assert actors(game)[aide]['position'] == (x, 9)
    game.screenshot('potion-' + label + '-offer')
    advance(game, lambda: var(game, 'lab_scene') == 2 and not script_active(game))
    assert potion_count(game) == before + 1
    assert actors(game)[aide]['position'] == (2, 9)
    assert game.position() == (x, 8) and not player_state(game)['frozen']
    game.step(UP)
    game.step(DOWN)
    assert not script_active(game) and potion_count(game) == before + 1
    return potion_count(game)


def check_battle(game, maps, species, lyra_species, index, checks):
    lab, town = maps['PolishedElmsLab'], maps['PolishedNewBarkTown']
    lyra = resident_id(lab, 'LOCALID_ELMS_LAB_LYRA')
    game.snapshot('before-lyra-battle')
    result = finish_battle(game, maps, species.lower(), expected=lyra_species)
    # A native save before the Potion preserves the pending handoff and Lyra's
    # departure, so neither battle nor prize can repeat after a reset.
    save_game(game)
    continue_game(game)
    assert var(game, 'lab_scene') == 5 and lyra not in actors(game)
    amount = take_potion(game, maps, 4 if index % 2 else 5, species.lower())
    save_game(game)
    continue_game(game)
    assert var(game, 'lab_scene') == 2 and lyra not in actors(game)
    assert potion_count(game) == amount
    result['potion_count'] = amount
    (game.output / ('battle-' + species.lower() + '.json')).write_text(json.dumps(result, indent=2) + '\n')
    # Cyndaquil's source Flame Body can defeat Lyra through burn damage even
    # when the player only switches. Use the actual Totodile starter branch
    # for a deliberate loss, preserving the source abilities and real inputs.
    if species == 'Totodile':
        game.snapshot('after-lyra-win')
        game.restore('before-lyra-battle')
        loss = finish_battle(game, maps, 'loss', lose=True, expected=lyra_species)
        take_potion(game, maps, 5, 'loss')
        save_game(game)
        continue_game(game)
        travel(game, lab, town)
        travel(game, town, lab)
        assert var(game, 'lab_scene') == 2 and lyra not in actors(game)
        (game.output / 'battle-loss.json').write_text(json.dumps(loss, indent=2) + '\n')
        game.restore('after-lyra-win')
        checks.append('Lyra loss continues in the lab, heals the entire party and saves her departure without whiteout or rematch')
    checks.append(f'Lyra uses Lv5 {lyra_species}; {species} branch wins, heals, grants one Potion and persists scenes 6/5/2')
    print('PASS: Lyra battle and Potion', species, flush=True)
