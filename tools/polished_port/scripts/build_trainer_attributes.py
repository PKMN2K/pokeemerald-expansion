#!/usr/bin/env python3
"""Import class attributes and bind every currently registered source trainer.

Source party-wide flags matter: once custom EVs/DVs/personality are enabled,
the macro's defaults replace class defaults for every member of that party.
"""
import argparse
import json
import re
import build_battle_suite as archive
import build_pokemon_base_data as species

ROOT = archive.ROOT
MANIFEST = ROOT/'tools/polished_port/manifests/trainer_attributes.json'
BINDINGS = {
    **{f'LYRA_{s}': ('LYRA1', f'LYRA1_{i}') for i, s in enumerate(('CHIKORITA', 'CYNDAQUIL', 'TOTODILE'), 1)},
    **{f'RIVAL_{s}': ('RIVAL0', str(i)) for i, s in enumerate(('CHIKORITA', 'CYNDAQUIL', 'TOTODILE'), 1)},
    'MIKEY': ('YOUNGSTER', 'MIKEY'), 'DON': ('BUG_CATCHER', 'DON'), 'JOEY': ('YOUNGSTER', 'JOEY1'),
    'WADE': ('BUG_CATCHER', 'WADE1'), 'FINCH': ('COOLTRAINERM', 'FINCH'),
    **{f'SPROUT_{s}': ('SAGE', s) for s in ('CHOW', 'NICO', 'EDMOND', 'JIN', 'TROY', 'NEAL')},
    'SPROUT_LI': ('ELDER', 'LI'), 'ROD': ('BIRD_KEEPER', 'ROD'), 'ABE': ('BIRD_KEEPER', 'ABE'),
    'FALKNER': ('FALKNER', '1'), 'GIOVANNI1': ('GIOVANNI', 'GIOVANNI1'),
}
STATS = ('HP', 'Atk', 'Def', 'Spe', 'SAt', 'SDf')
NATURES = {'ATK_UP_SATK_DOWN': 'ADAMANT', 'DEF_UP_SPE_DOWN': 'RELAXED', 'SATK_UP_SDEF_DOWN': 'RASH', 'SATK_UP_ATK_DOWN': 'MODEST'}


def spread(text, default):
    values = [default] * 6
    if text is not None:
        values = [0] * 6
        for value, stat in re.findall(r'(\d+) (HP|Atk|Def|Spe|SAt|SDf|All)\b', text):
            if stat == 'All': values = [int(value)] * 6
            else: values[STATS.index(stat)] = int(value)
    return values


def build():
    files = {p: r['text'] for p, r in archive.authenticated_sources().items()}
    plural_check = files['engine/battle/core.asm'].split('CheckPluralTrainer:', 1)[1].split('\n\tret', 1)[0]
    plural_classes = set(re.findall(r'^\s*cp (\w+)$', plural_check, re.M))
    classes = re.findall(r'^\s*trainerclass (\w+)', files['constants/trainer_constants.asm'].split('DEF NUM_TRAINER_CLASSES', 1)[0], re.M)[1:]
    assert plural_classes and plural_classes <= set(classes), 'Unexpected source trainer plurality checks'
    class_rows = [line for line in archive.source.default_lines(files['data/trainers/dvs.asm']) if line.startswith('db ')]
    assert len(classes) == len(class_rows)
    defaults = {}
    for cls, row in zip(classes, class_rows):
        ev, hpAtk, defSpe, satSdf, gender = row.replace('PERFECT_DVS', '$FF, $FF, $FF')[3:].split(',')
        dvs = [int(c, 16) for word in (hpAtk, defSpe, satSdf) for c in word.strip().removeprefix('$')]
        defaults[cls] = dict(evs=[int(ev)//3]*6, dvs=dvs, gender=gender.strip())
    attribute_lines = [line for line in archive.source.default_lines(files['data/trainers/attributes.asm'])
                       if line.startswith(('db ', 'dw '))]
    assert len(attribute_lines) == 4 * len(classes)
    for i, cls in enumerate(classes):
        items, reward, ai, switch = attribute_lines[i*4:i*4+4]
        defaults[cls].update(items=items[3:].split(', '), base_reward=int(reward[3:]),
            source_ai_layers=ai[3:].split(' + '), source_switch_flags=switch[3:].split(' + '))
    assert 'ldh a, [hMoveMon]' in files['engine/battle/read_trainer_dvs.asm']
    assert 'inc a\n\tld [de], a\n\tldh [hMoveMon], a' in files['engine/pokemon/move_mon.asm']
    assert 'MAX_RETURN_HAPPINESS  EQU 160' in files['constants/pokemon_data_constants.asm']
    parties = '\n'.join(archive.source.default_lines(files['data/trainers/parties.asm']))
    base = json.loads(species.build()[species.MANIFEST])['records']
    base = {(r['source_species'], r['source_form']): r for r in base}
    mapped = set(re.findall(r'#define TRAINER_POLISHED_(\w+)', (ROOT/'include/constants/opponents.h').read_text()))
    assert mapped == set(BINDINGS), 'New source trainer needs an authenticated attribute binding'
    records = []
    for target, (cls, identity) in BINDINGS.items():
        group = parties.split('def_trainer_class '+cls+'\n', 1)[1].split('def_trainer_class ', 1)[0]
        teams = re.findall(r'def_trainer (\w+), "([^"]+)"\n(.*?)end_trainer', group, re.S)
        ordinal, (_, name, body) = next((i, t) for i, t in enumerate(teams, 1) if t[0] == identity)
        customEV = 'tr_evs ' in body
        customDV = 'tr_dvs ' in body
        personality = bool(re.search(r'^tr_mon .*\b(?:MALE|FEMALE|GENDERLESS)\b', body, re.M))
        mons = []
        for slot, mon in enumerate(body.split('tr_mon ')[1:], 1):
            line = mon.splitlines()[0]
            match = re.fullmatch(r'(\d+|LEVEL_FROM_BADGES \+ \d+), (?:"[^"]+", )?(\w+)(?: @ \w+)?(?:, (MALE|FEMALE|GENDERLESS)(?: \| (\w+))?)?', line)
            assert match, line
            level, sp, gender, form = match.groups()
            data = base[(sp, form or 'PLAIN_FORM')]
            extra = re.search(r'^tr_extra (.*)', mon, re.M)
            nature, ability = 'HARDY', (0 if personality else ((slot ^ ordinal ^ (data['source_table_index'] & 255)) & 1))
            if extra:
                for token in extra[1].split(', '):
                    if token in NATURES: nature = NATURES[token]
                    else:
                        assert 'ABILITY_'+token in data['abilities'], token
                        ability = data['abilities'].index('ABILITY_'+token)
            ev = re.search(r'^tr_evs (.*)', mon, re.M)
            dv = re.search(r'^tr_dvs (.*)', mon, re.M)
            evs = spread(ev[1] if ev else None, 0) if customEV else defaults[cls]['evs'][:]
            dvs = spread(dv[1] if dv else None, 15) if customDV else defaults[cls]['dvs'][:]
            if 'GYRO_BALL' in mon:
                dvs[3] &= 1
                evs[3] = 0
            mons.append(dict(species=data['target_species'], source_species=sp, source_level=level,
                ivs=[2*d for d in dvs], evs=evs, ability_slot=ability, ability=data['abilities'][ability],
                nature='NATURE_'+nature, gender='GENDERLESS' if data['gender_ratio'] == 255 else gender or defaults[cls]['gender'], friendship=160))
        records.append(dict(target_trainer='TRAINER_POLISHED_'+target, source_class=cls, source_id=ordinal, source_name=name,
            items=['ITEM_'+s if s != '0' else 'ITEM_NONE' for s in defaults[cls]['items']],
            base_reward=defaults[cls]['base_reward'], mons=mons))
    out = ['/* Generated from authenticated default source trainer macros and class data. */', '#if !IS_FRLG',
           'static const struct PolishedTrainerEntry sPolishedTrainerEntries[] = {']
    for r in records:
        out += ['    {', f'        .trainer = {r["target_trainer"]}, .count = {len(r["mons"])}, .baseMoney = {r["base_reward"]},']
        if r['source_class'] in plural_classes:
            out.append('        .pluralName = TRUE,')
        out.append('        .items = {'+', '.join(r['items'])+'}, .mons = {')
        for mon in r['mons']:
            out += ['            {', f'                .species = {mon["species"]}, .abilitySlot = {mon["ability_slot"]},',
                    f'                .nature = {mon["nature"]}, .gender = TRAINER_MON_{"RANDOM_GENDER" if mon["gender"] == "GENDERLESS" else mon["gender"]},',
                    '                .iv = TRAINER_PARTY_IVS('+', '.join(map(str, mon['ivs']))+'),',
                    '                .ev = {'+', '.join(str(mon['evs'][i]) for i in (0,1,2,4,5,3))+'},', '            },']
        out += ['        },', '    },']
    out += ['};', '#endif', '']
    report = dict(source_commit=archive.source.PIN, source_variant='default; FAITHFUL undefined', classes=defaults,
        bindings=records, remaining='Unregistered source parties and source AI are not implemented by this adapter.')
    return {ROOT/'src/data/polished_trainers.h': '\n'.join(out), MANIFEST: json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, text in build().items():
        if args.check: assert path.read_text() == text, f'Regenerate {path}'
        else: path.write_text(text)
    print('PASS: source trainer class attributes and all registered party bindings')
