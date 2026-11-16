#!/usr/bin/env python3
"""Pin the source damaging-poison pool and its exclusion from Corrosion's bypass."""
import argparse
import json

import build_pokemon_learnsets as source
import build_corrosion_substitute as corrosion


def build():
    # Authenticates the exact direct-user whitelist: EFFECT_POISON/EFFECT_TOXIC.
    corrosion.build()
    files = source.sources()
    rows = [[v.strip() for v in line[5:].split(',')] for line in
            source.default_lines(files['data/moves/moves.asm']) if line.startswith('move ')]
    selected = []
    for identity, row in zip(source.move_identities(files)[1:], rows, strict=True):
        assert identity['source_move'] == row[0]
        if row[1] == 'EFFECT_POISON_HIT':
            assert row[3] == 'POISON' and row[6] == '30' and row[7] != 'STATUS'
            selected.append(dict(**identity, source_effect=row[1], secondary_percent=int(row[6])))
    assert [row['source_move'] for row in selected] == ['POISON_STING', 'GUNK_SHOT', 'POISON_JAB', 'SLUDGE_BOMB']
    effects = [line.split()[1] for line in source.default_lines(files['constants/move_effect_constants.asm'])
               if line.startswith('const EFFECT_')]
    pointers = [line.split()[1] for line in source.default_lines(files['data/moves/effects_pointers.asm'])
                if line.startswith('dw ')]
    assert dict(zip(effects, pointers, strict=True))['EFFECT_POISON_HIT'] == 'PoisonHit'
    scripts = files['data/moves/effects.asm']
    poison_hit = scripts.split('\nPoisonHit:\n', 1)[1].split('\nBurnHit:', 1)[0]
    assert [line for line in source.default_lines(poison_hit) if line] == [
        'checkobedience', 'usedmovetext', 'doturn', 'hastarget', 'checkhit',
        'checkpriority', 'critical', 'damagestats', 'damagecalc', 'stab',
        'damagevariation', 'moveanim', 'failuretext', 'applydamage',
        'criticaltext', 'supereffectivetext', 'postfainteffects', 'effectchance',
        'poisontarget', 'posthiteffects', 'endmove']
    commands = files['engine/battle/effect_commands.asm']
    target = commands.split('\nBattleCommand_poisontarget:\n', 1)[1].split('\nBattleCommand_paralyzetarget:', 1)[0]
    assert [line for line in source.default_lines(target) if line] == [
        'ld b, 0', 'call CanPoisonTarget', 'ret nz', 'ld b, 1 << PSN', 'jr StatusTarget']
    can_poison = commands.split('\nCanPoisonTarget:\n', 1)[1].split('\nCanBurnTarget:', 1)[0]
    assert [line for line in source.default_lines(can_poison) if line] == [
        'ld a, b', 'lb bc, POISON, STEEL', 'lb de, IMMUNITY, HELD_PREVENT_POISON',
        'ld h, 1 << PSN', 'jr CanStatusTarget']
    status = commands.split('\nCanStatusTarget:\n', 1)[1].split('\nBattleCommand_draintarget:', 1)[0]
    types = status.split('\n.not_corrosive\n', 1)[1].split('\n\tcall GetOpponentItemAfterUnnerve', 1)[0]
    assert [line for line in source.default_lines(types) if line] == [
        'push de', 'push bc', 'ld a, b', 'call CheckIfTargetIsSomeType',
        'pop bc', 'jr z, .cant_type', 'ld a, c', 'call CheckIfTargetIsSomeType',
        'jr z, .cant_type', 'ld a, [wTypeModifier]', 'and a', 'jr z, .cant_type']
    apply = commands.split('\nStatusTarget:\n', 1)[1].split('\nDisplayAndSetStatusProblem:', 1)[0]
    assert [line for line in source.default_lines(apply) if line] == [
        'ld a, [wTypeModifier]', 'and a', 'ret z', 'ld a, [wEffectFailed]', 'and a', 'ret nz']
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='The four source PoisonHit moves with Poison battle type in singles without active gimmicks.',
        moves=selected,
        rules=['PoisonHit applies damage before effectchance and poisontarget.',
               'poisontarget passes mode zero to CanPoisonTarget, which checks Poison and Steel immunity.',
               'Only EFFECT_POISON and EFFECT_TOXIC enter the direct-user Corrosion bypass; EFFECT_POISON_HIT takes the ordinary type and protection checks.',
               'Steel remains immune to Poison damage. Eligible susceptible targets may receive regular poison after a successful secondary roll.',
               'Existing status, ability protections, Substitute, Safeguard, hit and protection gates still apply.'],
        adapter='Use a generated source move-membership table at execution of the nonvolatile additional effect. For eligible poison effects, supply ABILITY_NONE instead of Corrosion only to the ordinary eligibility query, preserving the actual battler ability and shared AI queries.',
        gaps=['This verifies the Corrosion/type distinction and selected protection gates, not full secondary-effect RNG, rounding, post-hit ordering, AI decisions or presentation.',
              'Optional damaging poison moves, retyped moves, doubles and active gimmicks retain native paths. General status, hazard and optional combinations remain separate audits.'])
    header = f'/* Generated by build_poison_hit.py from Polished Crystal {source.PIN}. */\n'
    header += 'static const u8 sPolishedPoisonHitMoves[MOVES_COUNT] = {\n'
    header += ''.join(f"    [{row['target_move']}] = TRUE,\n" for row in selected)
    header += '};\n'
    return {
        source.ROOT/'src/data/polished_poison_hit.h': header,
        source.ROOT/'tools/polished_port/manifests/poison_hit.json': json.dumps(report, indent=2)+'\n',
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: four pinned PoisonHit moves retain type immunity with Corrosion')
