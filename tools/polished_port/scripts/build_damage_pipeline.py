#!/usr/bin/env python3
"""Authenticate and map the pinned damage pipeline and ability move lists."""
import argparse
import itertools
import json
import re
import build_battle_suite as archive

ROOT = archive.ROOT


def reference(level, attack, defense, attack_stage, defense_stage, critical):
    # DamagePass1..4 and the stage helpers, before STAB/weather/roll.
    while attack > 255 or defense > 255:
        attack, defense = max(1, attack >> 1), max(1, defense >> 1)
    value = (2 * level // 5 + 2) * 40 * attack
    stages = []
    if not critical or attack_stage >= 6:
        stages.append(attack_stage)
    if not critical or defense_stage < 6:
        stages.append(12 - defense_stage)
    for stage in stages:
        n, d = (stage - 4, 2) if stage >= 6 else (2, 8 - stage)
        value = (value & 0xffffff) * n // d
    if critical:
        value = (value & 0xffffff) * 3 // 2
    value = value // max(1, defense) // 50
    return 0xff00 if value >= 0xff00 else value + 2


def build():
    files = {p: r['text'] for p, r in archive.authenticated_sources().items()}
    _, moves, scripts = archive.attacks.inventory()
    mapping = {m['source_move']: m['target_move'] for m in moves}
    effect = files['engine/battle/effect_commands.asm']
    calc = effect.split('BattleCommand_damagecalc:', 1)[1].split('DamagePass1:', 1)[0]
    order = ['call DamagePass1', 'call DamagePass2', 'farcall ApplyDamageAbilities',
             '; Flash Fire', '; Critical hits', '; Item boosts', 'call GetOpponentItem', 'call DamagePass3']
    assert [calc.index(s) for s in order] == sorted(calc.index(s) for s in order)
    assert 'ln a, 9, 4' in calc and 'ln a, 13, 10' in calc and 'ln a, 2, 3' in calc
    assert 'call TruncateHL_BC' in effect
    assert 'ds 1\nhMultiplicand:: ds 3' in files['ram/hram.asm']
    assert 'cp HELD_METAL_POWDER' in effect and 'MON_SPECIES\n\tcall OpponentPartyAttr' in effect
    out = ['/* Generated from the pinned default Polished Crystal release. */',
           'static const u8 sPolishedDamageMoves[MOVES_COUNT] = {']
    active, deferred = [], []
    for m in moves:
        c = scripts[m['source_effect']]['commands']
        if 'damagecalc' in c:
            active.append(m['target_move'])
            out.append(f'    [{m["target_move"]}] = 1,')
    out += ['};', 'static const u8 sPolishedDamageFlags[MOVES_COUNT] = {']
    flags = {}
    for filename, flag in [('punching_moves', 1), ('launcher_moves', 2), ('slicing_moves', 4)]:
        for name in re.findall(r'^\s*db ([A-Z][A-Z_]+)', files[f'data/moves/{filename}.asm'], re.M):
            flags[mapping[name]] = flags.get(mapping[name], 0) | flag
    for m in moves:
        c = scripts[m['source_effect']]['commands']
        if 'effectchance' in c or 'selfeffectchance' in c:
            flags[m['target_move']] = flags.get(m['target_move'], 0) | 8
        if m['source_effect'] in ('EFFECT_RECOIL_HIT', 'EFFECT_JUMP_KICK') and m['source_move'] != 'STRUGGLE':
            flags[m['target_move']] = flags.get(m['target_move'], 0) | 16
    out += [f'    [{m}] = {v},' for m, v in sorted(flags.items())]
    out += ['};', 'static const bool8 sPolishedDamageAbilities[ABILITIES_COUNT] = {']
    abilities = re.findall(r'^\s*const ([A-Z][A-Z_0-9]+)', files['constants/ability_constants.asm'], re.M)
    constants = (ROOT/'include/constants/abilities.h').read_text()
    for name in abilities:
        name = 'NONE' if name == 'NO_ABILITY' else name
        assert 'ABILITY_' + name in constants, name
        out.append(f'    [ABILITY_{name}] = TRUE,')
    out += ['};', '']
    manifest = dict(source_commit=archive.source.PIN, source_variant='default release; FAITHFUL undefined',
                    damage_pipeline_moves=active, deferred_damage_pipeline_moves=deferred,
                    source_move_flags=flags, damage_order=order,
                    limits=['Arithmetic coverage does not establish all move branches, damage event order, or exact RNG stream.',
                            'Dynamic powers and Future Sight use the separately authenticated dynamic_damage contracts.',
                            'Optional moves, abilities, damage items, terrain, gimmicks and doubles keep native calculation.',
                            'AI uses the same numeric calculation, but source AI decision parity remains open.'])
    vectors = []
    for level, (attack, defense), astage, dstage, crit in itertools.product((1, 50, 100, 255),
            [(7,11),(101,73),(255,256),(256,255),(511,512),(1,1023),(1023,1),(65535,65535)],
            range(13), range(13), range(2)):
        vectors.append((level, attack, defense, astage, dstage, crit, reference(level, attack, defense, astage, dstage, crit)))
    header = ['/* Generated source arithmetic: 40 power, neutral type. */',
              'static const struct { u16 attack, defense, damage; u8 level, attackStage, defenseStage, critical; } sPolishedDamageVectors[] = {']
    header += [f'    {{{a}, {d}, {v}, {lv}, {sa}, {sd}, {c}}},' for lv,a,d,sa,sd,c,v in vectors]
    header += ['};', '']
    manifest['arithmetic_vectors'] = len(vectors)
    return {ROOT/'src/data/polished_damage.h': '\n'.join(out),
            ROOT/'test/battle/polished_damage_vectors.h': '\n'.join(header),
            ROOT/'tools/polished_port/manifests/damage_pipeline.json': json.dumps(manifest, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, text in build().items():
        if args.check:
            assert path.read_text() == text, f'Regenerate {path}'
        else:
            path.write_text(text)
    print('PASS: pinned damage order, move routing, ability and attack memberships')
