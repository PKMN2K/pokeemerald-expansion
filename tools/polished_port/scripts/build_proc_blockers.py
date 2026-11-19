#!/usr/bin/env python3
"""Pin Shield Dust versus Covert Cloak/Sheer Force for direct flinch procs."""
import argparse
import json

import build_pokemon_learnsets as source
import build_flinch_recharge as recharge
import build_proc_recharge as proc


def build():
    # Includes the complete direct flinch helper and effectchance, proving
    # that their ability/item checks are distinct rather than interchangeable.
    proc.build()
    files = source.sources()
    commands = files['engine/battle/effect_commands.asm']
    direct = commands.split('\n.flinch_up:\n', 1)[1].split('\nCheckStatHerbsAfterIntimidate:', 1)[0]
    assert 'cp SHIELD_DUST' in direct
    assert 'HELD_COVERT_CLOAK' not in direct and 'SHEER_FORCE' not in direct
    posthit = commands.split('\nBattleCommand_posthiteffects:', 1)[1].split('\nCheckStatHerbsAfterIntimidate:', 1)[0]
    assert recharge.lines(posthit.split('\n\tfarcall RunHitAbilities', 1)[0]) == [
        'call HandleRampage', 'call HasOpponentFainted', 'call nz, CheckSubstituteOpp',
        'ret nz', 'ld a, [wAttackMissed]', 'and a', 'ret nz']
    assert 'CheckSheerForceNegation' not in posthit and 'HELD_COVERT_CLOAK' not in posthit
    end = commands.split('\nCheckEndMoveEffects:', 1)[1]
    assert recharge.lines(end)[:5] == [
        'call GetFutureSightUser', 'ret nz', 'call HandleRampage',
        'call CheckSheerForceNegation', 'ret z']
    assert '; COVERT CLOAK\n\titem_attribute 4000, HELD_COVERT_CLOAK, 0, ITEM, ITEMMENU_NOUSE, ITEMMENU_NOUSE' in files['data/items/attributes.asm']
    rows = [[v.strip() for v in line[5:].split(',')] for line in recharge.lines(files['data/moves/moves.asm']) if line.startswith('move ')]
    effects = [line.split()[1] for line in recharge.lines(files['constants/move_effect_constants.asm']) if line.startswith('const EFFECT_')]
    pointers = [line.split()[1] for line in recharge.lines(files['data/moves/effects_pointers.asm']) if line.startswith('dw ')]
    mapping = dict(zip(effects, pointers, strict=True))
    scripts = files['data/moves/effects.asm']
    examples = [('THUNDERBOLT', 'EFFECT_PARALYZE_HIT', 'ParalyzeHit', 'paralyzetarget'),
                ('ICE_BEAM', 'EFFECT_FREEZE_HIT', 'FreezeHit', 'freezetarget'),
                ('FLAMETHROWER', 'EFFECT_BURN_HIT', 'BurnHit', 'burntarget')]
    common = ['checkobedience', 'usedmovetext', 'doturn', 'hastarget', 'checkhit',
              'checkpriority', 'critical', 'damagestats', 'damagecalc', 'stab',
              'damagevariation', 'moveanim', 'failuretext', 'applydamage',
              'criticaltext', 'supereffectivetext', 'postfainteffects', 'effectchance']
    for move, effect, label, command in examples:
        row = next(row for row in rows if row[0] == move)
        assert row[1] == effect and row[6] == '10' and mapping[effect] == label
        block = scripts.split('\n'+label+':\n', 1)[1].split('\n\n', 1)[0]
        assert recharge.lines(block) == common + [command, 'posthiteffects', 'endmove']
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Covert Cloak and Sheer Force versus successful held-item/Stench flinch procs in ordinary singles without either active gimmick.',
        rules=[
            'The source direct flinch helper checks Shield Dust after Mold Breaker. It does not check Covert Cloak or reject a held-item proc because the attacker has Sheer Force.',
            'Move effectchance separately rejects Covert Cloak and Sheer Force. A suppressed intrinsic secondary effect does not skip posthiteffects or its direct item flinch helper.',
            'Thunderbolt, Ice Beam and Flamethrower execute effectchance and their status command before posthiteffects; Sheer Force still suppresses their intrinsic status while permitting a successful held-item flinch.',
            'Active Inner Focus is rechecked when the target acts. Existing recharge clearing, before/after-action behavior and Steadfast rules apply to newly eligible procs.',
            'Moves already containing flinchtarget do not gain a second item/Stench roll. Intrinsic flinch and other secondary effects retain their separate Covert Cloak and Sheer Force checks.'],
        adapter='Use the explicit proc-origin marker and existing ordinary-singles gate to bypass only Covert Cloak and Sheer Force rejection in SetMoveEffect. Preserve Shield Dust blocking and recording, fainted-target/Substitute gates, and all unmarked effect paths.',
        gaps=[
            'This closes these two proc-blocker gaps, not full proc eligibility. Broken Substitute, attacker fainting to Rocky Helmet, multi-hit attacks, other item/ability combinations and optional content remain separate audits.',
            'Chance calculations/RNG stream, general recharge application, complete action order, AI and original presentation are not certified by these checks.',
            'Native-profile battles, doubles and either active gimmick retain native behavior. Optional damage and recharge moves inherit the scoped proc adapter without certification of their own effects.'])
    return {source.ROOT/'tools/polished_port/manifests/proc_blockers.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned direct flinch procs preserve Shield Dust but bypass Covert Cloak and Sheer Force')
