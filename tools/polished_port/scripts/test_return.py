#!/usr/bin/env python3
"""Audit the return trainers, source choreography and save allocations."""
import json
import re
from build_new_bark import ROOT, source_files


def main():
    files=source_files()
    party=(ROOT/'src/data/polished_return.party').read_text()
    source=files['data/trainers/parties.asm'].decode()
    group=source.split('Rival0Group:\n',1)[1].split('SECTION',1)[0]
    parties=re.findall(r'def_trainer (\d), "boy"\n(.*?)end_trainer',group,re.S)
    assert len(parties)==3
    for number,body in parties:
        mons=re.findall(r'tr_mon (\d+), (\w+)(?: @ (\w+))?',body)
        rival=mons[-1][1]
        live=party.split('=== TRAINER_POLISHED_RIVAL_'+rival+' ===')[1].split('=== TRAINER_')[0]
        actual=re.findall(r'^([A-Z][a-z]+) \(M\)(?: @ ([A-Za-z ]+))?\nLevel: (\d+)',live,re.M)
        assert [(lv,s.upper(),i.upper().replace(' ','_')) for s,i,lv in actual]==mons,(number,actual)
        assert live.count('30 HP / 30 Atk / 30 Def / 30 SpA / 30 SpD / 30 Spe')==2
        assert not re.search(r'^EVs:',live,re.M)
    joey=party.split('=== TRAINER_POLISHED_JOEY ===')[1]
    assert 'Rattata (M)\nLevel: 5\nAbility: Guts\nNature: Adamant' in joey
    learn=files['data/pokemon/evos_attacks.asm'].decode().split('evos_attacks RattataPlain\n')[1].split('evos_attacks ')[0]
    assert re.findall(r'^- (.+)',joey,re.M)==[n.title().replace('_',' ') for lv,n in re.findall(r'learnset (\d+), (\w+)',learn) if int(lv)<=5][-4:]
    source_city=files['maps/CherrygroveCity.asm'].decode()
    city=(ROOT/'data/maps/PolishedCherrygroveCity/scripts.inc').read_text()
    # Source exit falls through into another movement label: preserve the last two steps too.
    for original,ported in [('RivalWalksToYou','RivalApproach'),('RivalPushesYouOutOfTheWay','PlayerPush'),
            ('RivalStartsToLeave','RivalStarts'),('RivalComesBack','RivalBack'),('RivalExitsStageLeft','RivalExit')]:
        src=source_city.split('CherrygroveCity_'+original+':\n')[1].split('step_end')[0]
        live=city.split('PolishedReturn_'+ported+':\n')[1].split('step_end')[0]
        assert re.findall(r'^\s*(step|run_step|turn_head)_(\w+)',src,re.M)==[(dict(walk='step',walk_fast='run_step',face='turn_head')[op],d)
            for op,d in re.findall(r'^\s*(walk_fast|walk|face)_(\w+)',live,re.M)]
    constants=(ROOT/'include/constants/flags.h').read_text()+(ROOT/'include/constants/flags_frlg.h').read_text()
    flags=re.findall(r'#define (\w+)\s+(0x[\da-fA-F]+)',constants)
    for name,n in [('CHERRYGROVE_RIVAL_DONE',0x4bd),('ELM_THEFT_REPORTED',0x4be),('GAVE_MYSTERY_EGG',0x4bf)]:
        assert [f for f,v in flags if int(v,16)==n]==['FLAG_POLISHED_'+name]
    trainers=(ROOT/'include/constants/opponents.h').read_text()
    numbers=dict((n,int(v)) for n,v in re.findall(r'^#define (\w+)\s+(\d+)\s*(?://[^\n]*)?$',trainers,re.M))
    assert numbers['MAX_TRAINERS_COUNT_EMERALD']==864
    assert [numbers['TRAINER_POLISHED_'+n] for n in ('RIVAL_CHIKORITA','RIVAL_CYNDAQUIL','RIVAL_TOTODILE','JOEY')]==list(range(860,864))
    lab=json.loads((ROOT/'data/maps/PolishedElmsLab/map.json').read_text())
    coords=[(v['x'],v['y']) for v in lab['coord_events'] if int(v['var_value'])==3]
    assert coords==[(4,5),(5,5)]
    assert all(re.search(rf'coord_event\s+{x},\s*{y}, 3, MeetCopScript',files['maps/ElmsLab.asm'].decode()) for x,y in coords)
    print('PASS: three pinned Rival0 parties, Joey learnset and source rival exit choreography')
    print('PASS: both lab approaches and collision-free Emerald/FRLG save allocations')


if __name__=='__main__':
    main()
