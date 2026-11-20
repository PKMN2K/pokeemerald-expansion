#!/usr/bin/env python3
"""Authenticate delayed-attack contracts, source weights and dynamic powers."""
import argparse
import itertools
import json
import re
import build_battle_suite as archive
import build_pokemon_base_data as species

ROOT = archive.ROOT


def gyro(user, target):
    if not user:
        return 1
    while user > 255:
        user, target = user >> 1, target >> 1
    return min(150, max(1, 25 * target // user))


def build():
    files = {p: r['text'] for p, r in archive.authenticated_sources().items()}
    low = files['engine/battle/move_effects/low_kick.asm']
    assert 'farcall GetSpeciesWeight' in low and 'cp LIGHT_METAL' in low
    assert 'srl d\n\trr e' in low and 'jr nc, .loop2' in low
    bands = [(int(p), int(w)) for p, w in re.findall(r'dbw\s+(\d+),\s*(\d+)', files['data/moves/low_kick_power.asm'])]
    assert bands == [(120, 2000), (100, 1000), (80, 50), (60, 25), (40, 10), (20, 0)]
    body = files['data/pokemon/body_data.asm']
    weights = [int(w) for w in re.findall(r'^\s*body_data\s+\d+,\s*(\d+),', body, re.M)]
    records = json.loads(species.build()[species.MANIFEST])['records']
    mapped = {r['target_species']: weights[r['source_table_index'] - 1] for r in records}
    assert len(mapped) == 391 and all(mapped.values())
    speed = files['engine/battle/move_effects/gyro_ball.asm']
    assert 'srl b\n\trr c\n\tsrl d\n\trr e' in speed and 'ld a, 25' in speed
    assert 'cp 151' in speed and 'ld a, 1' in speed
    future = files['engine/battle/move_effects/future_sight.asm']
    assert 'or $3' in future and 'call HasUserFainted' in future
    effect = files['engine/battle/effect_commands.asm']
    assert 'call GetFutureSightUser\n\tjr z, .not_external_acc\n\tld b, 7' in effect
    assert 'call GetFutureSightUser\n\tld c, 0\n\tjr nz, .Ability' in effect
    assert 'farcall GetFutureSightUser\n\tret nc' in files['engine/battle/abilities.asm'].split('AnalyticAbility:', 1)[1]
    order = files['home/battle.asm'].split('CheckSpeed::', 1)[1]
    assert 'ld a, [wTrickRoom]' in order and 'xor 1' in order and 'call BattleRandom\n\tand $1' in order
    out = ['/* Generated from pinned Polished Crystal body data and Low Kick table. */',
           'static const u16 sPolishedBattleWeights[NUM_SPECIES] = {']
    out += [f'    [{s}] = {w},' for s, w in mapped.items()]
    out += ['};', 'static const struct { u16 threshold; u8 power; } sPolishedLowKickBands[] = {']
    out += [f'    {{{w}, {p}}},' for p, w in bands]
    out += ['};', '']
    speeds = [0, 1, 2, 3, 7, 24, 25, 26, 127, 128, 254, 255, 256, 257, 511, 512, 513, 999, 1023, 1024, 32767, 32768, 65535]
    vectors = [(a, d, gyro(a, d)) for a, d in itertools.product(speeds, repeat=2)]
    test = ['/* Source word/byte division boundary vectors, independent of runtime helper. */',
            'static const struct { u16 user, target; u8 power; } sPolishedGyroVectors[] = {']
    test += [f'    {{{a}, {d}, {p}}},' for a, d, p in vectors]
    test += ['};', 'static const struct { u16 species, weight; } sPolishedWeightVectors[] = {']
    test += [f'    {{{s}, {w}}},' for s, w in mapped.items()]
    test += ['};', '']
    manifest = dict(source_commit=archive.source.PIN, source_variant='default; FAITHFUL undefined',
        species_weights=mapped, low_kick_descending_bands=bands, gyro_vectors=len(vectors),
        contracts=['Low Kick selects the first strictly lower threshold; Light Metal halves source weight.',
                   'Gyro Ball scales both speeds while user speed exceeds 255; no native plus one.',
                   'Future Sight uses three end-turn ticks; user Speed order honors Trick Room and a fresh random tie.',
                   'External and fainted original users have no ability/item or attacking/accuracy stages.',
                   'Future Sight checks Protect, semi-invulnerability and accuracy at impact; bypasses Substitute.',
                   'Metronome and Analytic do not boost Future Sight; external critical chance starts at zero.'],
        remaining=['Light Metal on a weight-one target produces zero and cannot terminate the source table scan; retain native arithmetic rather than reproduce that hang.',
                   'Global residual/replacement order, ability and item trigger ordering, exact RNG and full move branch parity remain open.'])
    return {ROOT/'src/data/polished_dynamic_damage.h': '\n'.join(out),
            ROOT/'test/battle/polished_dynamic_damage_vectors.h': '\n'.join(test),
            ROOT/'tools/polished_port/manifests/dynamic_damage.json': json.dumps(manifest, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, content in build().items():
        if args.check:
            assert path.read_text() == content, f'Regenerate {path}'
        else:
            path.write_text(content)
    print('PASS: source species/form weights, Low Kick bands, Gyro Ball vectors and Future Sight contracts')
