#!/usr/bin/env python3
"""Import the four Battle Tower tiers from Polished Crystal's default build."""
import argparse
import json
import re

import build_trainer_roster as trainers

ROOT = trainers.ROOT
DATA = ROOT/'src/data/polished_tower_sets.h'
NATURES = dict(trainers.NATURES, ATK_UP_SPE_DOWN='BRAVE', SATK_UP_DEF_DOWN='MILD',
               DEF_UP_SATK_DOWN='IMPISH', SATK_UP_SPE_DOWN='QUIET', SDEF_UP_SPE_DOWN='SASSY')


def catalog():
    f = {k:r['text'] for k,r in trainers.archive.authenticated_sources().items()}
    default = trainers.archive.source.default_lines
    moves = trainers.archive.source.move_targets(trainers.archive.source.sources())
    base = {(r['source_species'],r['source_form']):r for r in json.loads(trainers.species.MANIFEST.read_text())['records']}
    names = re.findall(r'^\s*const (BTDVS_\w+)', f['constants/battle_tower_constants.asm'],re.M)
    spread = [tuple(2*int(c,16) for pair in re.findall(r'\$([0-9a-f]{2})',line) for c in pair)
              for line in default(f['data/battle_tower/dvs.asm']) if line.startswith('db ')]
    dvs = dict(zip(names,spread,strict=True))
    masks = re.findall(r'^\s*db %([01]{6}), %([01]{6})', f['data/battle_tower/tiers.asm'],re.M)
    tiers = []
    groups = '\n'.join(default(f['data/battle_tower/parties.asm'])).split('db -1')
    for group in groups[:-1]:
        sets = []
        for entry in re.split(r'^dp ',group,flags=re.M)[1:]:
            lines = entry.strip().splitlines()
            assert len(lines) == 5 and all(l.startswith('db ') for l in lines[1:]), lines
            sp, _, persona = lines[0].partition(', ')
            gender,_,form = persona.partition(' | ')
            if gender.endswith('_FORM') and not form:
                form, gender = gender, 'GENDERLESS'
            b = base[sp,form or 'PLAIN_FORM']
            item = lines[1][3:]
            ability,nature = lines[4][3:].split(' | NAT_')
            slots = [n for n,a in enumerate(b['abilities']) if ability.endswith('_'+a.removeprefix('ABILITY_'))]
            assert slots, (sp,ability)
            sets.append(dict(species=b['target_species'], base=base[sp,'PLAIN_FORM']['target_species'],
                             gender={0:'MALE',254:'FEMALE',255:'GENDERLESS'}.get(b['gender_ratio'],gender),
                             ability=slots[0], nature='NATURE_'+NATURES[nature], iv=dvs[lines[3][3:]],
                             item='ITEM_BLUNDER_POLICY' if item=='BLUNDRPOLICY' else trainers.item(item),
                             moves=[moves[m.strip()] for m in lines[2][3:].split(',')]))
        tiers.append(sets)
    assert [len(t) for t in tiers]==[61,140,143,88] and len(masks)==8
    return tiers, [[int(x,2) for x in row] for row in masks]


def build():
    tiers, masks = catalog()
    o = [trainers.MARK.replace('build_trainer_roster','build_tower_parties'),
         'static const u8 sTowerTierMasks[][2] = {']
    o.extend('    {'+f'{a}, {b}'+'},' for a,b in masks)
    o.extend(['};','static const u16 sTowerTierOffsets[] = {0, '+', '.join(str(sum(map(len,tiers[:i+1]))) for i in range(4))+'};',
              'static const struct PolishedTowerSet sTowerSets[] = {'])
    for n,tier in enumerate(tiers):
        o.append(f'    // Tier {"DCBA"[n]}')
        for s in tier:
            gender='RANDOM_GENDER' if s['gender']=='GENDERLESS' else s['gender']
            o.append(f'    {{{s["species"]}, {s["base"]}, {s["item"]}, {{{", ".join(s["moves"])}}}, '
                     f'TRAINER_PARTY_IVS({", ".join(map(str,s["iv"]))}), TRAINER_MON_{gender}, {s["nature"]}, {s["ability"]}}},')
    return '\n'.join(o+['};',''])


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--check',action='store_true')
    args=p.parse_args()
    text=build()
    if args.check:
        assert DATA.read_text()==text, f'Regenerate {DATA}'
    else:
        DATA.write_text(text)
    print('PASS: all 432 Battle Tower sets and eight difficulty rows')
