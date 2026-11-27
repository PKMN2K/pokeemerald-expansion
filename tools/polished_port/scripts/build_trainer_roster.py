#!/usr/bin/env python3
"""Translate every default-release trainer party, including Inver's seven candidates."""
import argparse
from collections import Counter
from functools import lru_cache
import json
import re

import build_battle_suite as archive
import build_pokemon_base_data as species
import build_pokemon_held_items as held
import build_trainer_attributes as legacy

ROOT = archive.ROOT
IDS = ROOT / 'include/constants/polished_trainer_ids.h'
DATA = ROOT / 'src/data/polished_trainer_parties.h'
ATTRS = ROOT / 'src/data/polished_trainer_roster.h'
CLASSES = ROOT / 'src/data/polished_trainer_classes.h'
CLASS_IDS = ROOT / 'include/constants/polished_trainer_classes.h'
MARK = '/* Generated from Polished Crystal v3.2.3 by build_trainer_roster.py. */'
ITEMS = dict(held.ALIASES, ARMOR_SUIT='POLISHED_ARMOR_SUIT', PROTECT_PADS='PROTECTIVE_PADS',
             WEAK_POLICY='WEAKNESS_POLICY')
NATURES = dict(legacy.NATURES, DEF_UP_ATK_DOWN='BOLD', SDEF_UP_ATK_DOWN='CALM',
               SDEF_UP_SATK_DOWN='CAREFUL', SPE_UP_ATK_DOWN='TIMID',
               SPE_UP_DEF_DOWN='HASTY', SPE_UP_SATK_DOWN='JOLLY', SPE_UP_SDEF_DOWN='NAIVE')
MON = re.compile(r'([0-9]+|LEVEL_FROM_BADGES [+-] [0-9]+), (?:"([^"]+)", )?(\w+)'
                 r'(?: @ (\w+))?(?:, (MALE|FEMALE|GENDERLESS))?(?: \| (\w+)|, (\w+_FORM))?')
STAT_NAMES = ('HP', 'Atk', 'Def', 'Spe', 'SAt', 'SDf')


def item(name):
    return 'ITEM_' + ITEMS.get(name, name)


def spread(text, default):
    result = [default] * 6
    if text:
        for field in text.split(', '):
            value, stat = field.split()
            if stat == 'All':
                result = [int(value)] * 6
            else:
                result[STAT_NAMES.index(stat)] = int(value)
    return result


def label(text):
    return 'TRAINER_CLASS_PC_' + re.sub(r'[^A-Z0-9]+', '_', text.upper()).strip('_')


def mon_record(section, defaults, flags, base, move_ids, hidden, ordinal, slot):
    head, *tail = section.splitlines()
    head = re.sub(r'\s*,\s*', ', ', head.strip())
    match = MON.fullmatch(head)
    assert match, head
    level, nickname, sp, held_item, gender, form, plain_form = match.groups()
    form = form or plain_form
    data = base[sp, form or 'PLAIN_FORM']
    attrs = {}
    for line in tail:
        if not line.strip():
            continue
        tag, args = line.split(' ', 1)
        assert tag in ('tr_moves', 'tr_extra', 'tr_dvs', 'tr_evs') and tag not in attrs, line
        attrs[tag] = args
    moves = attrs.get('tr_moves', '').split(', ') if 'tr_moves' in attrs else []
    dv_spec = attrs.get('tr_dvs')
    if dv_spec is None:
        dv_spec = next(('DVS_' + m for m in moves if m.startswith('HP_')), None)
    dvs = spread(hidden.get(dv_spec, dv_spec), 15) if flags['dvs'] else defaults['dvs'][:]
    evs = spread(attrs.get('tr_evs'), 0) if flags['evs'] else defaults['evs'][:]
    if 'GYRO_BALL' in moves:
        dvs[3] &= 1
        evs[3] = 0
    ability = 0 if flags['personality'] else ((slot ^ ordinal ^ base[sp, 'PLAIN_FORM']['source_table_index']) & 1)
    nature, shiny = 'HARDY', False
    for token in filter(None, attrs.get('tr_extra', '').split(', ')):
        if token == 'SHINY':
            shiny = True
        elif token in NATURES:
            nature = NATURES[token]
        else:
            ability = data['abilities'].index('ABILITY_' + token)
    ratio = data['gender_ratio']
    gender = {0:'MALE', 254:'FEMALE', 255:'GENDERLESS'}.get(ratio, gender or defaults['gender'])
    source_level = int(level) if level.isdigit() else 178 + int(level.split('LEVEL_FROM_BADGES')[1].replace(' ', ''))
    return dict(species=data['target_species'], level=source_level, nickname=nickname, item=item(held_item or 'NO_ITEM'),
                moves=[move_ids['HIDDEN_POWER' if m.startswith('HP_') else m] for m in moves],
                nature='NATURE_' + nature, shiny=shiny, ability=ability, gender=gender, ivs=[v*2 for v in dvs], evs=evs,
                regional_arbok=(sp == 'ARBOK' and form is None))


@lru_cache(maxsize=1)
def roster():
    f = {p:r['text'] for p,r in archive.authenticated_sources().items()}
    default = archive.source.default_lines
    classes = json.loads(legacy.build()[legacy.MANIFEST])['classes']
    names = re.findall(r'^li "([^"]+)"', '\n'.join(default(f['data/trainers/class_names.asm'])), re.M)
    genders = re.findall(r'^\s*dbit (MALE|FEMALE)_BIT', f['data/trainers/genders.asm'], re.M)
    assert len(classes) == len(names) and list(classes)[len(genders)-1] == 'FACTORYHEAD'
    genders.extend(c['gender'] for c in list(classes.values())[len(genders):])
    plural = f['engine/battle/core.asm'].split('CheckPluralTrainer:', 1)[1].split('\n\tret', 1)[0]
    plural_classes = set(re.findall(r'^\s*cp (\w+)$', plural, re.M))
    for (cls, data), name, gender in zip(classes.items(), names, genders, strict=True):
        data.update(name=name.replace('<PK><MN>', '{PKMN}').replace('#', 'Poké'), gender_class=gender, plural=cls in plural_classes)
    base = {(r['source_species'], r['source_form']): r for r in json.loads(species.build()[species.MANIFEST])['records']}
    move_ids = archive.source.move_targets(archive.source.sources())
    macro = f['data/trainers/macros.asm'].split('; Hidden Power DVs', 1)[1].split('MACRO dv_spread', 1)[0]
    hidden = dict(re.findall(r'DEF (DVS_HP_\w+)\s+EQUS "([^"]+)"', '\n'.join(default(macro))))
    parties = f['data/trainers/parties.asm'].replace('INCLUDE "data/trainers/psychic_inver.asm"', f['data/trainers/psychic_inver.asm'])
    text = '\n'.join(default(parties))
    groups = re.split(r'^def_trainer_class (\w+)\n', text, flags=re.M)
    known = {v:'TRAINER_POLISHED_'+k for k,v in legacy.BINDINGS.items()}
    records = []
    for cls, group in zip(groups[1::2], groups[2::2], strict=True):
        teams = re.findall(r'^def_trainer (\w+), "([^"]+)"\n(.*?)^end_trainer$', group, re.M|re.S)
        for ordinal, (identity, name, body) in enumerate(teams, 1):
            key = f'TRAINER_PC_{cls}_{identity}'
            explicit = re.search(r'^tr_mon .*\b(?:MALE|FEMALE|GENDERLESS)\b', body, re.M) is not None
            flags = dict(evs='tr_evs ' in body, dvs='tr_dvs ' in body or 'HP_' in body, personality=explicit)
            mons = [mon_record(part, classes[cls], flags, base, move_ids, hidden, ordinal, i)
                    for i,part in enumerate(body.split('tr_mon ')[1:], 1)]
            assert len(mons) <= (7 if cls == 'INVER' else 6), (cls, identity)
            records.append(dict(key=key, legacy=known.get((cls,identity)), cls=cls, ordinal=ordinal, name=name, mons=mons))
    assert len(records) == text.count('\ndef_trainer ') == 956
    assert {(r['cls'], r['ordinal']) for r in records if not r['mons']} == {
        ('GRUNTM',1), ('GRUNTM',27), ('TOWERTYCOON',1), ('FACTORYHEAD',1), ('FACTORYHEAD',2)}
    assert sum(len(r['mons']) for r in records) == 2659
    assert Counter(r['legacy'] for r in records if r['legacy']) == Counter(known.values())
    return classes, records


def ai_data(classes, records):
    files = {p:r['text'] for p,r in archive.authenticated_sources().items()}
    order = re.findall(r'^\s*fardw AI_(\w+)', files['engine/battle/ai/move.asm'], re.M)
    assert len(order) == 10
    masks = {'AI_'+name.upper():1<<i for i,name in enumerate(order)}
    masks['NO_AI'] = 0
    lines = [MARK, 'static const u16 sTrainerAiLayers[TRAINERS_COUNT] = {']
    for row in records:
        flags = classes[row['cls']]['source_ai_layers']
        lines.append(f'    [{row["key"]}] = {sum(masks[f] for f in flags)},')
    lines.extend(['};', 'static const u8 sSourceMovePolicies[MOVES_COUNT_ALL] = {'])
    moves = archive.source.move_targets(archive.source.sources())
    move_data = archive.source.default_lines(archive.source.sources()['data/moves/moves.asm'].split('Moves::',1)[1])
    effects = [line.split()[1] for line in archive.source.default_lines(
        archive.source.sources()['constants/move_effect_constants.asm']) if line.startswith('const EFFECT_')]
    up = set(effects[effects.index('EFFECT_ATTACK_UP'):effects.index('EFFECT_ATTACK_DOWN')])
    down = set(effects[effects.index('EFFECT_ATTACK_DOWN'):effects.index('EFFECT_EVASION_DOWN_2') + 1])
    zero, raise_stat, lower_stat = set(), set(), set()
    for line in move_data:
        match = re.match(r'move (\w+),\s*(\w+),\s*(\d+),', line)
        if not match:
            continue
        name, effect, power = match.groups()
        if int(power) == 0:
            zero.add(name)
        if effect in up:
            raise_stat.add(name)
        if effect in down:
            lower_stat.add(name)
    def move_list(file):
        return set(re.findall(r'^\s*db (\w+)\s*$', files['data/battle/ai/'+file+'_moves.asm'], re.M))
    stall, residual = move_list('stall'), move_list('residual')
    assert len(zero)>50 and len(stall)>20 and len(residual)==10
    groups = [('SOURCE_STATUS_MOVE', zero), ('SOURCE_STALL_MOVE', stall),
              ('SOURCE_RESIDUAL_MOVE', residual), ('SOURCE_RAISE_STAT', raise_stat),
              ('SOURCE_LOWER_STAT', lower_stat)]
    for move in sorted(set.union(*(group for _, group in groups))):
        flags = [name for name, group in groups if move in group]
        lines.append(f'    [{moves[move]}] = ' + ' | '.join(flags) + ',')
    return lines + ['};']


def build():
    classes, records = roster()
    ids = [MARK, '#ifndef GUARD_POLISHED_TRAINER_IDS_H', '#define GUARD_POLISHED_TRAINER_IDS_H', '']
    # Pin IDs in source order after the original named campaign and partner IDs.
    extra = [r for r in records if not r['legacy']]
    ids += ['#define POLISHED_ROSTER_COUNT ' + str(len(extra)),
            '#define POLISHED_ROSTER_FIRST (TRAINER_POLISHED_GIOVANNI1 + 1)', '']
    names = sorted(set(c['name'] for c in classes.values()))
    assert len(set(map(label, names))) == len(names)
    class_ids = [MARK, f'#define POLISHED_CLASS_COUNT {len(names)}']
    class_ids.extend(f'#define {label(n)} (TRAINER_CLASS_ELDER + {i})' for i,n in enumerate(names,1))
    class_init = [MARK] + [f'    [{label(n)}] = {{ _("{n}"), 0 }},' for n in names]
    defs, attrs = [MARK], [MARK, 'static const struct PolishedTrainerEntry sRosterEntries[] = {']
    rivals = []
    for r in records:
        if r['legacy']:
            ids.append(f'#define {r["key"]} {r["legacy"]}')
    for index, r in enumerate(extra):
        cls, key = classes[r['cls']], r['key']
        ids.append(f'#define {key} (POLISHED_ROSTER_FIRST + {index})')
        name = 'Silver' if r['name']=='<RIVAL>' else r['name']
        if r['name'] == '<RIVAL>':
            rivals.append(key)
        pic = r['cls']
        if pic == 'KIMONO_GIRL' and r['name'] != 'Naoko':
            pic += '_' + r['name'].upper()
        inv = [item('NO_ITEM' if x == '0' else x) for x in cls['items']]
        plural = cls['plural']
        attrs.extend([f'    [{key} - POLISHED_ROSTER_FIRST] = {{',
                      f'        .trainer = {key}, .count = {len(r["mons"])}, .baseMoney = {cls["base_reward"]},',
                      f'        .pluralName = {int(plural)}, .items = {{{", ".join(inv)}}}, .mons = {{'])
        defs.extend([f'    [DIFFICULTY_NORMAL][{key}] = {{',
                     f'        .trainerName = _("{name}"), .trainerClass = {label(cls["name"])},',
                     f'        .trainerPic = TRAINER_PIC_PC_{pic}, .gender = {cls["gender_class"]},',
                     f'        .partySize = {min(6,len(r["mons"]))}, .items = {{{", ".join(inv)}}},',
                     '        .party = (const struct TrainerMon[]) {'])
        for m in r['mons']:
            ev = ', '.join(str(m['evs'][i]) for i in (0,1,2,4,5,3))
            iv = ', '.join(map(str,m['ivs']))
            gender = 'RANDOM_GENDER' if m['gender']=='GENDERLESS' else m['gender']
            attrs.append(f'            {{.iv = TRAINER_PARTY_IVS({iv}), .species = {m["species"]}, .ev = {{{ev}}}, '
                         f'.abilitySlot = {m["ability"]}, .nature = {m["nature"]}, .gender = TRAINER_MON_{gender} }},')
            fields = [f'.species = {m["species"]}', f'.lvl = {m["level"]}']
            if m['item'] != 'ITEM_NONE':
                fields.append('.heldItem = '+m['item'])
            if m['moves']:
                fields.append('.moves = {'+', '.join(m['moves'])+'}')
            if m['nickname']:
                fields.append(f'.nickname = COMPOUND_STRING("{m["nickname"]}")')
            if m['shiny']:
                fields.append('.isShiny = TRUE')
            if m['regional_arbok']:
                fields.append('.tags = POLISHED_REGIONAL_ARBOK')
            defs.append('            {'+', '.join(fields)+'},')
        defs.extend(['        },','    },'])
        attrs.extend(['        },','    },'])
    attrs += ['};']
    assert rivals == [r['key'] for r in extra if r['cls'] in ('RIVAL1','RIVAL2')]
    ids += ['', '#endif']
    return {k:'\n'.join(v)+'\n' for k,v in ((IDS,ids),(DATA,defs),(ATTRS,attrs),(CLASSES,class_init),(CLASS_IDS,class_ids),(ROOT/'src/data/polished_trainer_ai.h',ai_data(classes,records)))}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, content in build().items():
        if args.check:
            assert path.read_text() == content, f'Regenerate {path}'
        else:
            path.write_text(content)
    print('PASS: 956 source trainer parties and 2,659 roster entries')
