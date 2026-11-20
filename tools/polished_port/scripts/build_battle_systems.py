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
    files = {p: v['text'] for p, v in archive.authenticated_sources().items()}
    balls = files['engine/items/poke_balls.asm']
    assert 'cp HIGH(1024)' in balls and 'jr nc, .heavymon' in balls
    assert 'cp 36' in balls and 'ln a, 1, 5' in balls
    assert 'ld b, a\n\tadd a\n\tadd b\n\tadd 10' in balls
    assert 'and EXTSPECIES_MASK\n\tcp [hl]\n\tret nz' in balls
    wobble = files['engine/battle_anims/pokeball_wobble.asm']
    bands = [(int(a), int(b)) for a, b in re.findall(r'^\s*db\s+(\d+),\s*(\d+)', wobble, re.M)]
    assert len(bands) == 115 and bands[0] == (1, 90) and bands[-1] == (255, 255)
    assert 'call Random\n\tcp b\n\tld c, 2' in wobble
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
        wobble_bands=bands, moon_ball_species=moon, counted_species=len(representatives),
        ownership_indices={r['target_species']: r['source_table_index'] for r in records},
        contracts=['Ball, catch-rate and status multipliers precede the divisions; HP compression floors at each step.',
                   'Heavy Ball mutates the byte catch rate across throws; high-byte band comparisons are inclusive.',
                   'Timer Ball performs its additions in an eight-bit register before its cap.',
                   'Love Ball compares masked user form to unmasked target form; valid nonzero target forms fail.',
                   'Normal captures use four byte-threshold checks; critical captures use one.',
                   'Default EXP is unscaled, with traded/trainer/Lucky Egg boosts before participant distribution.'])
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
