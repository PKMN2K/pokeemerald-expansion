#!/usr/bin/env python3
"""Keep exactly three completion suites and authenticate the lifecycle schedule."""
import argparse
import json
import re

import build_battle_suite as source

ROOT = source.ROOT
MANIFEST = ROOT / 'tools/polished_port/manifests/battle_completion_suites.json'
LIFECYCLE = ROOT / 'tools/polished_port/manifests/battle_lifecycle.json'
SUITE2_DOMAINS = frozenset(('actions', 'damage', 'moves', 'status', 'abilities', 'items', 'switching', 'fainting'))
SUITES = [
    dict(id=1, title='End-turn, entry and replacement foundations', status='implemented and regression-validated; domain closure pending',
         domains=['field'],
         contracts=['Source residual and timer phase order, per-phase Speed comparisons and faint checkpoints.',
                    'Shared entry hazard/item/ability order, deferred replacements and hazard KO loops.',
                    'Source residual arithmetic, Baton Pass state and entry status corrections with live regressions.']),
    dict(id=2, title='Action and attack resolution', status='implementation closed; documented source nontermination deviation',
         domains=['actions', 'damage', 'moves', 'status', 'abilities', 'items', 'switching', 'fainting'],
         contracts=['All branches of 133 effect families, 129 commands and 255 move identities.',
                    'Action ordering/cancellation, arithmetic, accuracy, statuses, abilities and items.',
                    'Complete voluntary/forced/pivot/Pursuit switching and action-triggered faint chains, including cross-phase effects using Suite 1 foundations.']),
    dict(id=3, title='Battle contexts and integration', status='planned',
         domains=['setup', 'capture_escape', 'rewards', 'ai', 'facility_link', 'presentation_rng'],
         contracts=['Trainer/wild setup and AI; capture/escape/roamers; rewards and saved outcomes.',
                    'Tower/link rules, RNG stream/call count, source presentation and observable order.',
                    'Full source/native, production, campaign and save validation; strict completion closure.']),
]

PHASES = {
    'ENDTURN_POLISHED_CHECKPOINT': ['CheckFaint'],
    'ENDTURN_WEATHER_DAMAGE': ['HandleWeather'],
    'ENDTURN_AFFECTION': ['HandleAffectionSelfCure'],
    'ENDTURN_FUTURE_SIGHT': ['HandleFutureSight'],
    'ENDTURN_FIRST_EVENT_BLOCK': ['HandleEndturnBlockA'],
    'ENDTURN_LEECH_SEED': ['HandleLeechSeed'],
    'ENDTURN_POISON': ['HandlePoison'],
    'ENDTURN_BURN': ['HandleBurn'],
    'ENDTURN_CURSE': ['HandleCurse'],
    'ENDTURN_WRAP': ['HandleWrap'],
    'ENDTURN_ENCORE': ['HandleEncore'],
    'ENDTURN_DISABLE': ['HandleDisable'],
    'ENDTURN_PERISH_SONG': ['HandlePerishSong'],
    'ENDTURN_ROOST': ['HandleRoost'],
    'ENDTURN_SECOND_EVENT_BLOCK': ['HandleReflect', 'HandleLightScreen', 'HandleSafeguard', 'HandleMist'],
    'ENDTURN_TRICK_ROOM': ['HandleTrickRoom'],
    'ENDTURN_THIRD_EVENT_BLOCK': ['HandleEndturnBlockB'],
    'ENDTURN_POLISHED_LEPPA': ['HandleLeppaBerry'],
    'ENDTURN_POLISHED_HEALING_ITEMS': ['HandleHealingItems'],
}


def validate_ownership(suites):
    assert [s['id'] for s in suites] == [1, 2, 3], 'Exactly three ordered suites required'
    domains = [d for s in suites for d in s['domains']]
    assert len(domains) == len(set(domains)), 'Duplicate domain ownership'
    assert set(domains) == set(source.DOMAINS), 'Missing or unknown domain ownership'
    assert all(s['contracts'] for s in suites)
    assert set(suites[1]['domains']) == SUITE2_DOMAINS, 'Suite 2 obligations cannot be reassigned to close the suite'


def validate_schedule(source_text, target_text, constants_text):
    body = source_text.split('HandleBetweenTurnEffects:', 1)[1].split('\n\t; these run even', 1)[0]
    calls = re.findall(r'^\s*call (\w+)\s*$', body, re.M)
    schedule = target_text.split('sPolishedEndTurnOrder[] =', 1)[1].split('};', 1)[0]
    phases = re.findall(r'\bENDTURN_[A-Z_0-9]+\b', schedule)
    native = re.findall(r'^\s*(ENDTURN_[A-Z_0-9]+),', constants_text.split('ENDTURN_COUNT,', 1)[0], re.M)
    assert all(phases.count(p) == 1 for p in native), 'Optional native phase lost or repeated'
    assert phases.index('ENDTURN_WEATHER') + 1 == phases.index('ENDTURN_WEATHER_DAMAGE')
    # The final checkpoint corresponds to .endturn_loop, after synchronization.
    projected = [call for phase in phases for call in PHASES.get(phase, [])]
    assert projected == calls + ['CheckFaint'], 'Source phase/checkpoint order changed'
    return calls, phases


def build():
    validate_ownership(SUITES)
    files = source.authenticated_sources()
    calls, phases = validate_schedule(files['engine/battle/endturn.asm']['text'],
        (ROOT / 'src/battle_end_turn.c').read_text(),
        (ROOT / 'include/constants/battle_end_turn.h').read_text())
    manifest = dict(source_commit=source.source.PIN,
        source_variant='default; FAITHFUL undefined',
        starting_target='7e88f8f9f2b68240951c97b2a222429283b7a302',
        complete_behavior_parity=False, suites=SUITES,
        effect_family_owner=2, source_move_count=255, source_effect_count=133, source_command_count=129,
        audit_inventory='tools/polished_port/manifests/battle_suite.json',
        suite2_implementation=dict(
            status='closed with the documented reachable source nontermination deviation',
            requested_from_target='8e3408022f98f85ec110bab6b9c1ce827b0c5492',
            resumed_from_target='2f2859623b217fa03b9b8fa916626ed77cfef7b8',
            scope='All eight original Suite 2 domains, 133 effect families, 129 commands and 255 source moves.',
            review='tools/polished_port/SUITE2_CLOSURE_REVIEW.md',
            validation='tools/polished_port/SUITE2_VALIDATION.json',
            reconciliations=['tools/polished_port/SUITE2_MOVE_RECONCILIATION.json',
                'tools/polished_port/SUITE2_ABILITY_ITEM_RECONCILIATION.md',
                'tools/polished_port/SUITE2_SWITCHING_FAINTING_RECONCILIATION.md'],
            implemented_checkpoints=[
                'Item backups, Choice and Metronome history, obedience/cancellation, caller PP, charging, forgetting and in-battle evolution boundaries.',
                'Damage/status/stat/critical ordering, survival, Rest/Roost/Transform, immediate healing and Enigma revival, Anger Point/thaw/Rage, reflection and priority failures.',
                'Selectable affection, classic categories and traded obedience; packed Leppa, medicines, self-hit item deferral, party identity, Room Service and Brick Break.',
                'Complete source move/effect/command, ability/item and switching/fainting reconciliation, including optional native controls and real Shift/wild faint input.',
                'Weakness Policy source phase partition: 162 early, seven final-hit late and 86 absent moves; Mirror Herb, caps, zero damage, Parental Bond, Future Sight and attacker-KO consequences.'],
            inventory_exclusions='Nineteen unused effect constants have no default source move; selectable options are included.',
            deliberate_deviation='Weight-one Light Metal can fail to terminate in the source. The target deliberately terminates; this reachable input is neither an inventory exclusion nor a parity pass.',
            evidence_policy='Authenticated branch interpretations and live bindings are reconciled with distinguishing evidence. Counts alone do not establish closure; no Suite 2 mechanical obligation was reassigned.'),
        next_step=dict(
            suite_id=3,
            goal='Complete the original battle-context and integration suite.',
            completion_status='planned',
            plan='tools/polished_port/BATTLE_PARITY_PLAN.md',
            domains=SUITES[2]['domains'],
            work=[
                'Finish trainer/wild construction, special contexts and source option UI; source AI scoring, information, selection, items and switching.',
                'Finish capture/escape/roamers, rewards, learning/evolution options, postbattle ability dispatch and saved outcomes.',
                'Finish Tower/link rules, construction, restrictions and synchronization while preserving optional native capabilities.',
                'Reconcile exact shared RNG stream/call counts and source messages, artwork, animations, timing and observable sequence.',
                'Run source/native, production, campaign and Save/reset/Continue validation; reconcile remaining whole-battle obligations and the explicit deviation before changing strict completeness.'],
            acceptance=[
                'Keep exactly three suites and their original primary domain ownership.',
                'Retain Suite 2 source anchors, runtime bindings and branch evidence; newly discovered Suite 2 defects remain owned by Suite 2.',
                'Whole-battle completeness requires reviewed evidence for every remaining obligation; passing catalogs and inventory counts alone cannot certify it.']),
        closure_policy='No suite closes until every owned obligation has authenticated source, live bindings and distinguishing branch evidence. Passing regression tests alone does not certify parity.')
    paths = ['engine/battle/endturn.asm', 'engine/battle/core.asm', 'engine/battle/abilities.asm',
             'engine/battle/effect_commands.asm', 'home/battle.asm']
    lifecycle = dict(source_commit=source.source.PIN, complete_behavior_parity=False,
        source_anchors={p: files[p]['sha'] for p in paths},
        source_calls=calls, target_phases=phases,
        contracts=[
            'Finish both battlers in each residual phase before faint animations, EXP and outcome checks.',
            'New Speed comparisons for each source phase and each screen effect; Trick Room expires before block B.',
            'Final Leppa and healing items run after block B. Replacements follow residuals and items.',
            'SubtractHP/PostStatus can trigger healing immediately; the final item sweep is additional. Unnerve exit immediately runs Leppa and healing sweeps.',
            'Magic Guard freezes Toxic; Poison Heal advances it even at full HP; burn is maxHP/16 without Heatproof halving.',
            'Non-Poison Black Sludge ignores Magic Guard. Seed restores half nominal drain after Big Root and Ooze ignores receiver Magic Guard.',
            'Baton Pass preserves binding/Toxic counters, inherits a trap from the opponent, and frees the departing users Mean Look target; source CANT_RUN is stored on the trap user.',
            'Both sides finish hazards and entry items before entry abilities; Spikes precede Toxic Spikes. Replacements repeat after hazard KOs.',
            'Ordinary and item-forced Toxic Spikes entry synchronize before berries; only Roar-style forced-target entry skips reflection and applies Mold Breaker to hazards. Substitute blocks the returned status.',
            'Simultaneous campaign knockouts count as losses. Optional battle contexts keep native behavior.'],
        live_bindings=[
            dict(source=['HandleBetweenTurnEffects', 'CheckFaint', 'HandleHealingItems', 'HandleLeppaBerry'],
                 target=['src/battle_end_turn.c', 'src/battle_move_resolution.c', 'data/battle_scripts_1.s'],
                 tests=['test/battle/polished_lifecycle.c']),
            dict(source=['SpikesDamageBoth', 'SpikesDamage', 'HandleEntryItems', 'RunBothEntryAbilities', 'SuppressUserAbilities'],
                 target=['src/battle_switch_in.c', 'src/battle_end_turn.c', 'src/battle_script_commands.c'],
                 tests=['test/battle/polished_lifecycle.c', 'test/battle/polished_switch_lifecycle.c', 'test/battle/polished_entry_status.c']),
            dict(source=['SendInUserPkmn', 'NewBattleMonStatus', 'FaintUserPokemon'],
                 target=['src/battle_main.c', 'src/battle_script_commands.c'],
                 tests=['test/battle/polished_switch_lifecycle.c'])],
        suite2_interaction_review='tools/polished_port/SUITE2_CLOSURE_REVIEW.md',
        remaining=['Whole-battle field-domain closure remains separate from the implemented Suite 1 foundations and reconciled Suite 2 interactions.',
                   'Exact RNG stream, source messages/animations/timing and Tower/link-specific contracts belong to Suite 3.'])
    return {MANIFEST: json.dumps(manifest, indent=2)+'\n', LIFECYCLE: json.dumps(lifecycle, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, content in build().items():
        if args.check:
            assert path.read_text() == content, f'Regenerate {path}'
        else:
            path.write_text(content)
    print('PASS: exactly three suites own all fifteen domains; source lifecycle schedule authenticated')
