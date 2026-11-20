#!/usr/bin/env python3
"""Authenticate shared battle-system contracts and generate source fixtures."""
import argparse
import json
import re
import build_battle_suite as archive
import build_pokemon_base_data as species
import build_pokemon_evolutions as evolutions

ROOT = archive.ROOT


def build():
    authenticated = archive.authenticated_sources()
    files = {p: v['text'] for p, v in authenticated.items()}
    balls = files['engine/items/poke_balls.asm']
    assert 'cp HIGH(1024)' in balls and 'jr nc, .heavymon' in balls
    assert 'cp 36' in balls and 'ln a, 1, 5' in balls
    assert 'ld b, a\n\tadd a\n\tadd b\n\tadd 10' in balls
    assert 'and EXTSPECIES_MASK\n\tcp [hl]\n\tret nz' in balls
    wobble = files['engine/battle_anims/pokeball_wobble.asm']
    bands = [(int(a), int(b)) for a, b in re.findall(r'^\s*db\s+(\d+),\s*(\d+)', wobble, re.M)]
    assert len(bands) == 115 and bands[0] == (1, 90) and bands[-1] == (255, 255)
    assert 'call Random\n\tcp b\n\tld c, 2' in wobble
    core = files['engine/battle/core.asm']
    rewards = core.split('GiveExperiencePoints:', 1)[1].split('GiveBattleEVs:', 1)[0]
    assert rewards.index('; Boost Experience for traded Pokemon') < rewards.index('; Boost experience for a trainer battle') < rewards.index('; Boost experience for Lucky Egg') < rewards.index('call .MaybeScaleExp')
    assert 'cp BATTLETYPE_SAFARI\n\tjmp z, SafariBattleTurn ; do not send out a player mon' in core
    assert 'and POKERUS_MASK' in core and 'WONTFIX because this has helpful synergy with EXP Share.' in core
    assert 'ld hl, wBattleMonItem' in core.split('GiveBattleEVs:', 1)[1]
    assert 'EVS_OPT_MODERN | (1 << RTC_OPT) | (1 << EVOLVE_IN_BATTLE_OPT)' in files['data/options/default_options.asm']
    assert 'SCALED_EXP_OPT' not in files['data/options/default_options.asm']
    payouts = [int(n) for n in re.findall(r'^\s*db (\d+)\s*$', files['engine/events/whiteout.asm'].split('.BasePayouts\n', 1)[1].split('DetermineWildBattlePanic:', 1)[0], re.M)]
    assert payouts == [8, 16, 24, 36, 48, 64, 80, 100, 120]
    records = json.loads(species.build()[species.MANIFEST])['records']
    assert max(r['source_table_index'] for r in records) == 337
    evos = json.loads(evolutions.build()[evolutions.MANIFEST])['records']
    moon = [r['target_species'] for r in evos if r['rules'] and
            r['rules'][0]['method'] == 'ITEM' and r['rules'][0]['argument'] == 'MOON_STONE']
    groups = list(dict.fromkeys(r['source_species'] for r in records))
    representatives = [next(r['target_species'] for r in records if r['source_species'] == s) for s in groups]
    thresholds = [0] + [next(b for a, b in reversed(bands) if a <= rate) for rate in range(1, 256)]
    out = ['/* Generated from the authenticated default Polished Crystal source. */',
           'static const u8 sPolishedCaptureWobble[256] = {']
    for i in range(0, 256, 16):
        out.append('    ' + ', '.join(map(str, thresholds[i:i+16])) + ',')
    out += ['};', 'static const bool8 sPolishedMoonBallSpecies[NUM_SPECIES] = {']
    out += [f'    [{s}] = TRUE,' for s in moon]
    out += ['};', 'static const u16 sPolishedCaptureDexSpecies[] = {']
    out += [f'    {s},' for s in representatives]
    out += ['};', 'static const u16 sPolishedCaptureForm[NUM_SPECIES] = {']
    out += [f'    [{r["target_species"]}] = {r["source_table_index"]},' for r in records]
    out += ['};', '']
    manifest = dict(source_commit=archive.source.PIN, source_variant='default; FAITHFUL undefined',
        source_anchors={p: authenticated[p]['sha'] for p in (
            'engine/items/poke_balls.asm', 'engine/items/item_effects.asm',
            'engine/battle_anims/pokeball_wobble.asm', 'engine/battle/core.asm',
            'engine/events/whiteout.asm', 'data/options/default_options.asm')},
        wobble_bands=bands, moon_ball_species=moon, counted_species=len(representatives),
        whiteout_base_payouts=payouts,
        ownership_indices={r['target_species']: r['source_table_index'] for r in records},
        contracts=['Ball, catch-rate and status multipliers precede the divisions; HP compression floors at each step.',
                   'Heavy Ball mutates the byte catch rate across throws; high-byte band comparisons are inclusive.',
                   'Timer Ball performs its additions in an eight-bit register before its cap.',
                   'Love Ball compares masked user form to unmasked target form; valid nonzero target forms fail.',
                   'Normal captures use four byte-threshold checks; critical captures use one.',
                   'Default EXP is unscaled, with traded/trainer/Lucky Egg boosts before participant distribution.',
                   'Safari has no participating player Pokemon; only living held Exp. Share recipients qualify.',
                   'Safari bait/anger mutate the catch-rate byte; reaction counters update before the escape decision.',
                   'EV bonuses use the active player item and active Pokerus, then the default 252/510 caps.',
                   'Whiteout loss uses the highest party level and the source badge-count payout capped at eight badges.'],
        complete_behavior_parity=False,
        remaining=['Capture preflight restrictions, tutorial flow, Safari UI/end sequence and complete party/box acquisition flows need end-to-end validation; delivery and Flash ownership probes are scoped evidence.',
                   'Level-one growth, all post-battle happiness/persistence, run/roaming, source AI and global residual/faint sequencing remain open.',
                   'Matching RNG thresholds does not establish identical random-stream state, draw count or presentation timing.'])
    return {ROOT/'src/data/polished_capture.h': '\n'.join(out),
            ROOT/'tools/polished_port/manifests/battle_systems.json': json.dumps(manifest, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, content in build().items():
        if args.check:
            assert path.read_text() == content, f'Regenerate {path}'
        else:
            path.write_text(content)
    print('PASS: authenticated shared battle-system contracts')
