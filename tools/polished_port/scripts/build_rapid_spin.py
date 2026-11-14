#!/usr/bin/env python3
"""Verify pinned Rapid Spin cleanup independently of Sheer Force secondary effects."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    moves = [line for line in source.default_lines(files['data/moves/moves.asm']) if line.startswith('move RAPID_SPIN,')]
    assert len(moves) == 1
    row = [v.strip() for v in moves[0][5:].split(',')]
    assert row == ['RAPID_SPIN', 'EFFECT_RAPID_SPIN', '50', 'NORMAL', '100', '20', '100', 'PHYSICAL']
    script = files['data/moves/effects.asm'].split('\nRapidSpin:', 1)[1].split('\n\tendmove', 1)[0]
    order = ['\tfailuretext', '\tapplydamage', '\tpostfainteffects', '\tselfeffectchance', '\traisestathit SPEED', '\tposthiteffects', '\tclearhazards']
    offsets = [script.index(command) for command in order]
    assert offsets == sorted(offsets) and script.strip().endswith('clearhazards')
    commands = files['engine/battle/effect_commands.asm']
    chance = commands.split('\nBattleCommand_selfeffectchance:', 1)[1].split('\nBattleCommand_lowersub:', 1)[0]
    assert 'cp SHEER_FORCE\n\tjr z, EffectChanceFailed' in chance
    assert 'EffectChanceFailed:\n\tld a, 1\n\tld [wEffectFailed], a' in chance
    assert '_RaiseStatHit:\n\tor STAT_CANMISS | STAT_SECONDARY | STAT_SILENT' in commands
    stats = files['engine/battle/stats.asm']
    assert 'bit STAT_SECONDARY_F, b\n\tjr z, .no_secondary\n\tld a, [wEffectFailed]\n\tand a\n\tret nz' in stats
    failure = commands.split('\nBattleCommand_failuretext:', 1)[1].split('\nBattleCommand_applydamage:', 1)[0]
    assert 'ld a, [wAttackMissed]\n\tand a\n\tret z' in failure
    assert 'cp EFFECT_DOUBLE_HIT\n\tjmp nz, EndMoveEffect' in failure
    posthit = commands.split('\nBattleCommand_posthiteffects:', 1)[1].split('\nCheckStatHerbsAfterIntimidate:', 1)[0]
    assert 'call HasUserFainted\n\tjr nz, .check_parental_bond\n\tcall EndMoveEffect' in posthit
    cleanup = files['engine/battle/move_effects/rapid_spin.asm']
    assert 'res SUBSTATUS_LEECH_SEED, [hl]' in cleanup
    assert 'ld hl, wPlayerHazards\n\tld de, wPlayerWrapCount' in cleanup
    assert 'ld hl, wEnemyHazards\n\tld de, wEnemyWrapCount' in cleanup
    assert 'and HAZARDS_SPIKES\n\tjr z, .no_spikes\n\tcpl\n\tand [hl]\n\tld [hl], a' in cleanup
    assert 'and HAZARDS_TOXIC_SPIKES\n\tjr z, .no_toxic_spikes\n\txor a\n\tld [hl], a' in cleanup
    assert 'pop de\n\tld a, [de]\n\tand a\n\tret z\n\txor a\n\tld [de], a' in cleanup
    assert 'SHEER_FORCE' not in cleanup and 'wEffectFailed' not in cleanup
    end = commands.split('\nCheckEndMoveEffects:', 1)[1].split('\nCheckThroatSpray:', 1)[0]
    assert 'call CheckSheerForceNegation\n\tret z' in end
    assert 'call nz, EndMoveDamageChecks' in end
    items = commands.split('\nEndMoveDamageChecks:', 1)[1]
    assert 'cp HELD_LIFE_ORB\n\tjr z, .life_orb' in items and 'cp HELD_SHELL_BELL\n\tjr z, .shell_bell' in items
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary singles Rapid Spin cleanup under Sheer Force, with successful/failed-hit, Substitute, faint, repeat and selected suppression controls.',
        move=dict(power=50, type='NORMAL', accuracy=100, pp=20, category='PHYSICAL', speed_secondary_chance=100, speed_stages=1),
        sheer_force=dict(suppresses_speed=True, suppresses_cleanup=False, suppresses_life_orb_and_shell_bell=True),
        cleanup=['user Leech Seed', 'all user-side Spikes layers', 'all user-side Toxic Spikes layers', 'user binding move'],
        gates=['Move failure ends the effect script before cleanup.', 'Post-hit processing aborts the remaining script when the user faints.', 'An intact or broken target Substitute does not prevent the separate cleanup command.'],
        adapter='Run the existing cleanup script once before resuming the native Sheer Force skip destination, for successful ordinary Rapid Spin damage by a living user. Keep Speed and other suppressed end effects suppressed. Native profiles, other moves, doubles and active gimmicks retain their paths; no new state or IDs.',
        gaps=['Native cleanup message/animation order is retained; original source presentation remains separate.',
              'Complete contact/faint/switch/item/ability scheduling, called/reflected/multi-hit moves, full Sheer Force damage/AI/RNG rules and hazard/trapping mechanics remain separate audits.',
              'Expansion-only hazards, formats and gimmicks remain available without certifying them as source behavior.'])
    return {source.ROOT/'tools/polished_port/manifests/rapid_spin.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Rapid Spin cleanup, Sheer Force separation and hit/faint gates')
