#!/usr/bin/env python3
"""Check rock behavior contracts against the pinned assembly and live wiring."""
import json
import re
from build_rock_smash import ROOT, sources, build


def main():
    f={p:b.decode() for p,b in sources().items() if p.endswith('.asm')}
    for path,data in build().items(): assert path.read_bytes()==data, path
    flow=f['engine/events/overworld.asm']
    check=flow.split('CheckPartyMove:',1)[1].split('\nHas',1)[0]
    assert 'MON_IS_EGG' in check and 'CanLearnTMHMMove' in check and 'MON_HP' not in check
    rock=flow.split('HasRockSmash:',1)[1].split('TryHeadbutt',1)[0]
    assert 'lb de, ROCK_SMASH, TM_ROCK_SMASH' in rock
    ask=flow.split('AskRockSmashScript:',1)[1].split('HasRockSmash:',1)[0]
    assert ask.index('HasRockSmash') < ask.index('ENGINE_ROCK_SMASH_ACTIVE') < ask.index('yesorno')
    auto=flow.split('AutoRockSmashScript:',1)[1].split('AskRockSmashScript:',1)[0]
    assert auto.index('disappear LAST_TALKED') < auto.index('RockMonEncounter') < auto.index('RockItemEncounter')
    assert 'reloadmapafterbattle\n\tend' in auto and 'earthquake 84' in auto and 'rock_smash 10' in auto
    assert 'DARK_CAVE_VIOLET_ENTRANCE, TREEMON_SET_ROCK' in f['data/wild/treemon_maps.asm']
    resets=f['engine/overworld/warp_connection.asm'].split('ResetOWMapState:',1)[1].split('\n\n',1)[0]
    assert 'wOWState' in resets and 'wEventFlags' in resets
    setup=f['data/maps/setup_scripts.asm']
    assert 'HandleNewMap' not in setup.split('MapSetupScript_ReloadMap:',1)[1].split('MapSetupScript_',1)[0]
    assert 'HandleContinueMap' in setup.split('MapSetupScript_Continue:',1)[1].split('MapSetupScript_',1)[0]
    menu=f['engine/pokemon/mon_submenu.asm'].split('GetMonSubmenuItems:',1)[1].split('ResetMonSubmenu:',1)[0]
    assert 'MON_MOVES' in menu and 'MON_IS_EGG' in menu and 'CanLearnTMHMMove' not in menu
    live=json.loads((ROOT/'data/maps/PolishedDarkCaveVioletEntrance/map.json').read_text())
    rocks=[o for o in live['object_events'] if o['graphics_id']=='OBJ_EVENT_GFX_POLISHED_ROCK']
    assert len(rocks)==4
    assert [(o['x'],o['y']) for o in rocks]==[(16,14),(27,6),(7,14),(36,31)]
    assert [o['flag'] for o in rocks]==['FLAG_TEMP_'+str(n) for n in range(1,5)]
    assert all(o['script']=='PolishedRockSmash_Interact' for o in rocks)
    script=(ROOT/'data/scripts/polished_rock_smash.inc').read_text()
    assert script.index('PolishedRockSmashPrepare') < script.index('goto_if_eq VAR_TEMP_5')
    assert script.index('removeobject') < script.index('special PolishedRockSmashEncounter')
    assert 'additem VAR_0x8000, 1' in script and 'checkfieldmove' not in script
    print('PASS: pinned rock eligibility, field/menu flows, temporary state, four live objects and source outputs')

if __name__=='__main__':main()
