#!/usr/bin/env python3
"""Pin Magic Bounce, Substitute exceptions and source pre-reflection priority gates."""
import argparse
import json
import re

import build_pokemon_learnsets as source

TABLE = source.ROOT / 'src/data/polished_magic_bounce.h'
MANIFEST = source.ROOT / 'tools/polished_port/manifests/magic_bounce.json'
EXPECTATIONS = source.ROOT / 'include/test/polished_magic_bounce.h'


def reflection_pool(files):
    constants = [line.split()[1] for line in source.default_lines(files['constants/move_effect_constants.asm'])
                 if line.startswith('const EFFECT_')]
    pointers = [line.split()[1] for line in source.default_lines(files['data/moves/effects_pointers.asm'])
                if line.startswith('dw ')]
    assert len(constants) == len(pointers) == 152 and len(set(constants)) == 152
    effects = dict(zip(constants, pointers, strict=True))
    parts = re.split(r'^([A-Za-z][A-Za-z0-9_]*):[^\n]*\n', files['data/moves/effects.asm'], flags=re.M)
    scripts = dict(zip(parts[1::2], parts[2::2], strict=True))
    # The Fly label aliases the immediately following Dig script.
    assert re.search(r'^Fly:\nDig:', files['data/moves/effects.asm'], re.M)
    assert not scripts['Fly'].strip()
    scripts['Fly'] = scripts['Dig']
    rows = [[v.strip() for v in line[5:].split(',')] for line in
            source.default_lines(files['data/moves/moves.asm'].split('Moves::', 1)[1]) if line.startswith('move ')]
    records = []
    for identity, row in zip(source.move_identities(files)[1:], rows, strict=True):
        assert identity['source_move'] == row[0]
        label = effects[row[1]]
        commands = [line for line in source.default_lines(scripts[label]) if line]
        # Reject unexpected script fallthrough or branching instead of assuming
        # that a label alone identifies all commands executed by a move.
        assert commands[-1] == 'endmove', label
        assert not any(line.startswith(('jump', 'call', 'INCLUDE')) for line in commands), label
        assert commands.count('bounceback') <= 1, label
        records.append(dict(**identity, source_effect=row[1], source_script=label,
                            reflection_candidate='bounceback' in commands))
    assert len(records) == 255 and sum(r['reflection_candidate'] for r in records) == 31
    return records


def move_list(files, path):
    rows = [line[3:].strip() for line in source.default_lines(files[path]) if line.startswith('db ')]
    assert rows[-1] == '-1' and len(rows) == len(set(rows))
    return rows[:-1]


def sound_priority_pool(files):
    sounds = set(move_list(files, 'data/moves/sound_moves.asm'))
    moves = {row[0]: row for row in [[v.strip() for v in line[5:].split(',')] for line in
             source.default_lines(files['data/moves/moves.asm']) if line.startswith('move ')]}
    parts = re.split(r'^([A-Za-z][A-Za-z0-9_]*):[^\n]*\n', files['data/moves/effects.asm'], flags=re.M)
    scripts = dict(zip(parts[1::2], parts[2::2], strict=True))
    result = []
    for row in reflection_pool(files):
        if row['source_constant'] not in sounds or moves[row['source_move']][7] != 'STATUS':
            continue
        commands = [line for line in source.default_lines(scripts[row['source_script']]) if line]
        if 'checkpriority' not in commands:
            continue
        assert moves[row['source_move']][3] == 'NORMAL'
        assert commands.index('checkhit') < commands.index('checkpriority') < commands.index('bounceback')
        result.append(row['source_constant'])
    assert set(result) == {'GROWL', 'ROAR', 'SING', 'SUPERSONIC', 'SCREECH'}
    priorities = {line[3:].split(',')[0].strip() for line in
                  source.default_lines(files['data/moves/priorities.asm']) if line.startswith('db ')}
    assert set(result) & priorities == {'ROAR'}  # The other four use the default priority zero.
    return result


def pre_reflection_priority_pool(files):
    moves = {row[0]: row for row in [[v.strip() for v in line[5:].split(',')] for line in
             source.default_lines(files['data/moves/moves.asm']) if line.startswith('move ')]}
    parts = re.split(r'^([A-Za-z][A-Za-z0-9_]*):[^\n]*\n', files['data/moves/effects.asm'], flags=re.M)
    scripts = dict(zip(parts[1::2], parts[2::2], strict=True))
    result, exceptions = [], []
    for row in reflection_pool(files):
        if not row['reflection_candidate']:
            continue
        commands = [line for line in source.default_lines(scripts[row['source_script']]) if line]
        if 'checkpriority' not in commands:
            exceptions.append(row['source_constant'])
            continue
        assert commands.count('checkpriority') == 1
        assert commands.index('checkpriority') < commands.index('bounceback')
        assert moves[row['source_move']][7] == 'STATUS'
        assert row['source_effect'] != 'EFFECT_DOUBLE_HIT'
        result.append(dict(source_move=row['source_constant'], target_move=row['target_move'],
                           source_type=moves[row['source_move']][3]))
    assert len(result) == 29 and set(exceptions) == {'SPIKES', 'TOXIC_SPIKES'}
    failure = files['engine/battle/effect_commands.asm'].split('\nBattleCommand_failuretext:', 1)[1].split('\nBattleCommand_applydamage:', 1)[0]
    assert 'cp EFFECT_DOUBLE_HIT\n\tjmp nz, EndMoveEffect' in failure
    return result


def build():
    files = source.sources()
    bypass = move_list(files, 'data/moves/substitute_bypass_moves.asm')
    sounds = move_list(files, 'data/moves/sound_moves.asm')
    assert bypass == ['ATTRACT', 'DISABLE', 'ENCORE', 'FORESIGHT', 'SPIKES', 'TOXIC_SPIKES']
    assert sounds == ['BUG_BUZZ', 'DISARM_VOICE', 'GROWL', 'HYPER_VOICE', 'PERISH_SONG',
                      'ROAR', 'SCREECH', 'SING', 'SUPERSONIC', 'HEAL_BELL']
    bounce = files['engine/battle/move_effects/magic_bounce.asm']
    gates = ['call GetOpponentAbilityAfterMoldBreaker\n\tcp MAGIC_BOUNCE\n\tret nz',
             'cp ATKFAIL_PROTECT\n\tret z', 'call CheckHiddenOpponent\n\tret nz',
             'ld hl, SubstituteBypassMoves', 'call CheckSubstituteOpp\n\tret nz',
             'bit SUBSTATUS_IN_ABILITY, a\n\tret nz', 'farcall BeginAbility', 'farcall EndAbility',
             'set SUBSTATUS_IN_ABILITY, [hl]']
    offsets = [bounce.index(gate) for gate in gates]
    assert offsets == sorted(offsets)
    commands = files['engine/battle/effect_commands.asm']
    checkhit = commands.split('BattleCommand_checkhit:\n', 1)[1].split('\n\tcall .DreamEater', 1)[0]
    assert list(line for line in source.default_lines(checkhit) if line) == [
        'ld a, BATTLE_VARS_SUBSTATUS2', 'call GetBattleVar', 'bit SUBSTATUS_IN_ABILITY, a', 'ret nz']
    assert bounce.index('set SUBSTATUS_IN_ABILITY, [hl]') < bounce.index('call ResetTurn') < bounce.index('res SUBSTATUS_IN_ABILITY, [hl]')
    sub = commands.split('\nCheckSubstituteOpp:', 1)[1].split('\nCheckUserMove:', 1)[0]
    assert 'call GetTrueUserAbility\n\tcp INFILTRATOR\n\tret z' in sub
    assert 'ld a, [wInAbility]\n\tand a\n\tjr nz, .no_sound_move' in sub
    assert 'ld hl, SoundMoves\n\tcall IsInByteArray' in sub
    assert 'jr nc, .no_sound_move\n\txor a\n\tret' in sub
    assert 'bit SUBSTATUS_SUBSTITUTE, a\n\tret' in sub
    identities = source.move_identities(files)[1:]
    targets = source.move_targets(files)
    exceptions = set(bypass + sounds)
    assert len(identities) == 255 and len(exceptions) == 16
    assert exceptions <= targets.keys()
    rows = [dict(**row, bypass_substitute=row['source_constant'] in exceptions)
            for row in reflection_pool(files)]
    table = f'/* Generated by build_magic_bounce.py from Polished Crystal {source.PIN}. */\n'
    table += 'static const u8 sPolishedMagicBounceSubstituteBypass[MOVES_COUNT] = {\n'
    table += ''.join(f"    [{targets[name]}] = TRUE,\n" for name in sorted(exceptions))
    table += '};\n'
    priority_sounds = sound_priority_pool(files)
    table += '\nstatic const u8 sPolishedSoundPriorityMoves[MOVES_COUNT] = {\n'
    table += ''.join(f'    [{targets[name]}] = TRUE,\n' for name in priority_sounds)
    table += '};\n'
    priority_moves = pre_reflection_priority_pool(files)
    table += '\n/* Zero excludes a move; source types are stored plus one. */\n'
    table += 'static const u8 sPolishedPreReflectionPriorityTypes[MOVES_COUNT] = {\n'
    table += ''.join(f'    [{row["target_move"]}] = TYPE_{row["source_type"]} + 1,\n' for row in priority_moves)
    table += '};\n'
    header = f'/* Source expectations from Polished Crystal {source.PIN}; generated by build_magic_bounce.py. */\n'
    header += '#ifndef GUARD_TEST_POLISHED_MAGIC_BOUNCE_H\n#define GUARD_TEST_POLISHED_MAGIC_BOUNCE_H\n\n'
    header += 'static const struct { enum Move move; bool8 candidate; } sSourceMagicBounceMoves[] = {\n'
    header += ''.join(f'    {{{r["target_move"]}, {"TRUE" if r["reflection_candidate"] else "FALSE"}}},\n' for r in rows)
    header += '};\n\n#endif\n'
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Static reflection candidacy for all 255 mapped source moves; ordinary singles Magic Bounce activation, Substitute eligibility, returned-move accuracy, five Normal-type sound-status priority gates and pre-reflection Prankster Dark immunity for 29 source-typed moves without active gimmicks.',
        explicit_exceptions=bypass, sound_exceptions=sounds, sound_priority_moves=priority_sounds,
        pre_reflection_priority_moves=priority_moves, moves=rows,
        reflection_candidates=31, reflection_non_candidates=224,
        rules=['The default source move table selects an effect constant, the indexed pointer selects its script, and only scripts containing bounceback are reflection candidates.',
               'Protect failure and a hidden Fly/Dig target prevent reflection before the Substitute check.',
               'Ordinary reflection is blocked by Substitute unless the move is in the explicit bypass or sound list, or its original user has Infiltrator.',
               'Roar is a sound exception: Substitute does not prevent its reflection.',
               'Mold Breaker suppresses Magic Bounce; reflected moves cannot bounce again.',
               'Magic Bounce sets IN_ABILITY around ResetTurn, so the returned move skips a new accuracy roll and retains the original wAttackMissed result. An original miss is not converted to a hit; ordinary later actions clear the result and check accuracy.',
               'Twenty-nine source reflection candidates check priority before bounceback; their Dark immunity prevents Magic Bounce activation. Spikes and Toxic Spikes omit checkpriority.',
               'Growl, Roar, Sing, Supersonic and Screech check priority before reflection. The checkpriority Prankster branch checks Dark immunity and returns before its Soundproof check; Sing additionally runs checkpowder/CheckNullificationAbilities earlier, so Soundproof still blocks Sing. Positive-priority Armor Tail blocks the four priority-zero moves after Prankster raises them.'],
        adapter='Retain the matching native reflection-candidate accessor, verified against independently generated source expectations for all 255 moves. Use the pinned PP registry for source membership and a generated 16-entry Substitute exception table. Track the reflection origin in an existing battle-only padding bit so source Magic Bounce skips returned accuracy without changing Magic Coat. Derive the five Normal-type sound status moves whose scripts check priority before reflection; apply source Prankster Dark/Soundproof ordering only during execution. Derive source types for all 29 candidates with pre-reflection priority checks and apply their Dark immunity before Magic Bounce in singles without active gimmicks. The 24 newly covered moves retain native Magic Coat ordering and shared AI estimates.',
        gaps=['This historical static-candidacy adapter does not certify whole move or ability parity. The later Suite 2 command reconciliation and action_resolution manifest own original miss/immunity preservation, manual miss routing and script-specific nullification evidence.',
              'The exact shared source RNG stream and call-state parity, AI and source presentation retain their original integration owners.',
              'Optional moves, doubles, active gimmicks and Magic Coat retain their documented native boundaries; the consolidated Suite 2 review records the distinguishing execution controls.'])
    return {TABLE: table, EXPECTATIONS: header, MANIFEST: json.dumps(report, indent=2) + '\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Magic Bounce pool (31 candidates / 224 exclusions), Substitute gates, six explicit exceptions and ten source sound moves')
