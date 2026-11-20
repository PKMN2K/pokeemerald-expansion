#!/usr/bin/env python3
"""Verify the remaining three source contact-table abilities and their gates."""
import argparse
import json

import build_contact_status as contact


def build():
    contact.build()
    source = contact.attacks.source
    files = source.sources()
    abilities = files['engine/battle/abilities.asm']
    table = abilities.split('TargetContactAbilities:\n', 1)[1].split('CuteCharmAbility:', 1)[0]
    assert all(f'dbw {name}, ' in table for name in ('CUTE_CHARM', 'TANGLING_HAIR', 'PERISH_BODY'))
    cute = abilities.split('CuteCharmAbility:\n', 1)[1].split('PerishBodyAbility:', 1)[0]
    assert 'call HasUserFainted\n\tret z' in cute
    assert 'ld a, 10\n\tcall BattleRandomRange\n\tcp 3\n\tret nc' in cute
    assert 'farcall BattleCommand_attract' in cute
    attract = files['engine/battle/move_effects/attract.asm']
    assert 'call CheckOppositeGender\n\tjr c, .failed\n\tjr z, .failed' in attract
    assert 'call CheckHiddenOpponent\n\tjr nz, .failed' in attract
    assert 'bit SUBSTATUS_IN_LOVE, [hl]\n\tjr nz, .failed' in attract
    assert 'cp OBLIVIOUS' in attract and 'cp HELD_DESTINY_KNOT' in attract
    assert 'call CheckMentalHerb\n\tfarjp RunStatusHealAbilities' in attract
    assert 'CheckSubstituteOpp' not in attract
    hair = abilities.split('TanglingHairAbility:\n', 1)[1].split('EffectSporeAbility:', 1)[0]
    assert 'call HasOpponentFainted\n\tret z' in hair
    assert 'ld b, SPEED\n\tld a, STAT_SILENT\n\tfarcall _ForceLowerOppStat' in hair
    assert 'farjp CheckMirrorHerb' in hair
    stats = files['engine/battle/stats.asm']
    assert 'call HasOpponentFainted\n\tret z\n.player\n\tcall HasUserFainted\n\tret z' in stats
    assert 'farcall CheckSubstituteOpp' in stats
    assert all(token in stats for token in ('cp CONTRARY', 'and GUARD_MIST', 'cp CLEAR_BODY', 'cp WHITE_SMOKE', 'cp HELD_CLEAR_AMULET'))
    perish = abilities.split('PerishBodyAbility:\n', 1)[1].split('TanglingHairAbility:', 1)[0]
    assert 'call z, SwapHLDE' in perish  # The current perspective is the defender.
    assert 'ld a, [hl]\n\tand a\n\tret nz' in perish
    assert 'ld [hl], 4' in perish and 'ld a, [de]\n\tand a\n\tjr nz, .no_user' in perish
    assert 'ld a, 4\n\tld [de], a' in perish
    assert 'HasUserFainted' not in perish and 'HasOpponentFainted' not in perish
    assert 'cp SOUNDPROOF' not in perish
    endturn = files['engine/battle/endturn.asm'].split('HandlePerishSong:\n', 1)[1].split('HandleTrickRoom:', 1)[0]
    assert 'call HasUserFainted\n\tret z' in endturn
    assert 'dec [hl]' in endturn and 'predef_jump SubtractHPFromUser' in endturn
    suppressed = files['data/abilities/mold_breaker_suppressed_abilities.asm']
    assert all(name not in suppressed for name in ('CUTE_CHARM', 'TANGLING_HAIR', 'PERISH_BODY'))
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)',
        scope='Mapped source moves in ordinary singles without either active gimmick; Cute Charm, Tangling Hair and Perish Body only.',
        contact_classification='contact_status.json: 255 identities; 124 direct attacks replayed against each ability on both sides.',
        rules=[
            'Post-hit processing permits an exhausted Substitute but rejects an intact one. Contact uses the source classifier, Protective Pads and Punching Glove.',
            'Cute Charm requires a living holder, opposite non-genderless Pokemon and no existing infatuation; it rolls three in ten independently of native generation presets. Oblivious, Destiny Knot and Mental Herb keep their scoped effects.',
            'Tangling Hair uses the shared forced opponent stat drop. Both Pokemon must be alive, the attacker Substitute blocks it, and the shared Contrary, Mist, Clear Body, White Smoke, Clear Amulet and stat-limit gates apply.',
            'Perish Body checks the attacker counter first. An existing attacker count prevents activation, even if the holder lacks one. Otherwise it starts the attacker count and adds a holder count only when missing.',
            'Perish Body ignores Soundproof and has no holder-faint rejection. Source counter four translates to native timer three, with countdown fainting on the fourth end turn including activation.',
            'Switching clears Perish state. Repeated contact and multi-hit strikes do not refresh existing counts. Poison Touch precedes these defender contact abilities.',
            'These three abilities are absent from the source Mold Breaker suppression table. Optional Neutralizing Gas, Long Reach, Aroma Veil, Gooey, formats and gimmicks retain native support.'],
        gaps=[
            'This completes the scoped follow-up to the contact-status table, not all ability or contact-engine parity. Aftermath, damaging items, global faint/recoil order, all curing/reflection chains, all stat reactions and deferred switches need separate audits.',
            'Exact source RNG stream, presentation, infatuation action timing, full end-turn ordering and AI estimates remain open. Native tests of optional mechanics are regression controls, not source certification.'])
    return {source.ROOT/'tools/polished_port/manifests/contact_volatile.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: Cute Charm, Tangling Hair and Perish Body source contact, faint, stat and countdown gates')
