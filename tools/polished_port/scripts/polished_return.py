"""Pinned return errand: Elm's call, Cherrygrove rival and lab investigation."""
import re
import tempfile
from pathlib import Path

from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png
from new_bark_intro import coord, movement, obj
from polished_route29 import object_at
from polished_route30 import palette


STORY = {
    'CherrygroveCity': {'source': 'CherrygroveRivalTriggerNorth/South', 'rows': [6, 7],
        'battle': 'Rival0 1/2/3, both outcomes continue and heal; scene 1 to 2'},
    'ElmsLab': {'source': 'MeetCopScript/MeetCopScript2, CopScript, SpecialNameRival, ElmAfterTheftScript',
        'columns': [4, 5], 'handoff': 'name rival; take one Mystery Egg; scene 3 to 2'},
    'Route30': {'source': 'ElmPhoneScript2.disaster, TrainerYoungsterJoey',
        'call': 'First outdoor return from Mr. Pokémon; clears on completion',
        'post_egg': 'Replace staged battle with sight trainer Joey; phone/rematches deferred'},
}


def assets(files):
    result = {}
    assert 'dba Rival0Pic' in files['data/trainers/pic_pointers.asm'].decode()
    assert 'Rival0Pic:\nRival1Pic: INCBIN "gfx/trainers/rival1.2bpp.lz"' in files['gfx/trainers.asm'].decode()
    assert 'INCLUDE "gfx/trainers/rival1.pal"\nINCLUDE "gfx/trainers/rival1.pal"' in files['data/trainers/palettes.asm'].decode()
    with tempfile.TemporaryDirectory() as tmp:
        src, dst = Path(tmp)/'rival.png', Path(tmp)/'out.png'
        src.write_bytes(files['gfx/trainers/rival1.png'])
        w, h, pixels = _read_png_2bit_grayscale(src)
        assert (w, h) == (56, 56)
        canvas = [[0]*64 for _ in range(64)]
        for y, row in enumerate(pixels):
            canvas[y+8][4:60] = [3-v for v in row]
        _write_indexed_4bit_png(dst, canvas)
        result['graphics/trainers/front_pics/polished_rival.png'] = dst.read_bytes()
    colors = [tuple(map(int, v)) for v in re.findall(r'RGB\s+(\d+),\s*(\d+),\s*(\d+)', files['gfx/trainers/rival1.pal'].decode())]
    assert len(colors) == 2
    result['graphics/trainers/palettes/polished_rival.pal'] = palette(colors)
    parties = files['data/trainers/parties.asm'].decode()
    group = parties.split('Rival0Group:')[1].split('SECTION')[0]
    assert '0, PERFECT_DVS,   MALE   ; rival0' in files['data/trainers/dvs.asm'].decode()
    code = '/* Pinned first rival parties: DV 15 -> IV 30, zero EVs. */\n'
    for index, body in re.findall(r'def_trainer (\d), "boy"\n(.*?)end_trainer', group, re.S):
        mons = re.findall(r'tr_mon (\d+), (\w+)(?: @ (\w+))?', body)
        assert mons[0] == ('4', 'RATTATA', '') and mons[1][0] == '5'
        species = mons[1][1]
        code += f'\n=== TRAINER_POLISHED_RIVAL_{species} ===\nName: boy\nClass: Rival\nPic: Polished Rival\nGender: Male\nMusic: Intense\nDouble Battle: No\n'
        for level, mon, item in mons:
            name = 'RattataPlain' if mon == 'RATTATA' else mon.title()
            learnset = re.search(r'evos_attacks '+name+r'\n(.*?)(?=\n\s*evos_attacks |\Z)', files['data/pokemon/evos_attacks.asm'].decode(), re.S)[1]
            moves = [move for lv, move in re.findall(r'learnset (\d+), (\w+)', learnset) if int(lv) <= int(level)][-4:]
            code += f'\n{mon.title()} (M)' + (' @ '+item.replace('_',' ').title() if item else '') + f'\nLevel: {level}\nIVs: 30 HP / 30 Atk / 30 Def / 30 SpA / 30 SpD / 30 Spe\n'
            code += ''.join('- '+move.replace('_',' ').title()+'\n' for move in moves)
    assert code.count('=== TRAINER_') == 3
    joey = re.search(r'def_trainer JOEY1, "Joey"\n(.*?)end_trainer', parties, re.S)[1]
    assert 'tr_mon 5, RATTATA, MALE' in joey and 'GUTS, ATK_UP_SATK_DOWN' in joey and 'tr_dvs 15 All' in joey
    code += '''
=== TRAINER_POLISHED_JOEY ===
Name: Joey
Class: Youngster
Pic: Polished Youngster
Gender: Male
Music: Male
Double Battle: No

Rattata (M)
Level: 5
Ability: Guts
Nature: Adamant
IVs: 30 HP / 30 Atk / 30 Def / 30 SpA / 30 SpD / 30 Spe
- Tackle
- Leer
- Quick Attack
'''
    result['src/data/polished_return.party'] = code.encode()
    return result


def city(script, header, text, define):
    rival = object_at('LOCALID_CHERRYGROVE_RIVAL', 'POLISHED_RIVAL', 39, 6, 'PolishedReturn_Silent', 'FLAG_TEMP_2')
    rival['movement_type'] = 'MOVEMENT_TYPE_FACE_LEFT'
    header['object_events'].append(rival)
    for y, label in ((6, 'North'), (7, 'South')):
        assert re.search(rf'coord_event 33,\s+{y}, 1, CherrygroveRivalTrigger{label}', script)
        coord(header, 33, y, 'VAR_POLISHED_CHERRYGROVE_SCENE', 1, 'PolishedReturn_Rival'+label)
    code = '''PolishedReturn_CityLoad::
	setflag FLAG_TEMP_2
	end
PolishedReturn_Silent::
	end
PolishedReturn_RivalSouth::
	lockall
	setobjectxyperm LOCALID_CHERRYGROVE_RIVAL, 39, 7
	goto PolishedReturn_RivalStart
PolishedReturn_RivalNorth::
	lockall
	setobjectxyperm LOCALID_CHERRYGROVE_RIVAL, 39, 6
PolishedReturn_RivalStart::
	turnobject OBJ_EVENT_ID_PLAYER, DIR_EAST
	applymovement OBJ_EVENT_ID_PLAYER, PolishedIntro_Exclamation
	waitmovement 0
	delay 15
	clearflag FLAG_TEMP_2
	addobject LOCALID_CHERRYGROVE_RIVAL
	applymovement LOCALID_CHERRYGROVE_RIVAL, PolishedReturn_RivalApproach
	waitmovement 0
	turnobject OBJ_EVENT_ID_PLAYER, DIR_EAST
	msgbox PolishedReturn_CherrygroveRivalText_Seen, MSGBOX_DEFAULT
	closemessage
	setvar VAR_LAST_TALKED, LOCALID_CHERRYGROVE_RIVAL
	goto_if_eq VAR_POLISHED_STARTER, 2, PolishedReturn_RivalChikorita
	goto_if_eq VAR_POLISHED_STARTER, 3, PolishedReturn_RivalCyndaquil
	trainerbattle_earlyrival TRAINER_POLISHED_RIVAL_TOTODILE, RIVAL_BATTLE_HEAL_AFTER, PolishedReturn_RivalCherrygroveWinText, PolishedReturn_RivalCherrygroveLossText
	goto PolishedReturn_RivalAfter
PolishedReturn_RivalChikorita::
	trainerbattle_earlyrival TRAINER_POLISHED_RIVAL_CHIKORITA, RIVAL_BATTLE_HEAL_AFTER, PolishedReturn_RivalCherrygroveWinText, PolishedReturn_RivalCherrygroveLossText
	goto PolishedReturn_RivalAfter
PolishedReturn_RivalCyndaquil::
	trainerbattle_earlyrival TRAINER_POLISHED_RIVAL_CYNDAQUIL, RIVAL_BATTLE_HEAL_AFTER, PolishedReturn_RivalCherrygroveWinText, PolishedReturn_RivalCherrygroveLossText
PolishedReturn_RivalAfter::
	lockall
	msgbox PolishedReturn_CherrygroveRivalTextAfter1, MSGBOX_DEFAULT
	closemessage
	playse SE_M_COMET_PUNCH
	applymovement OBJ_EVENT_ID_PLAYER, PolishedReturn_PlayerPush
	waitmovement 0
	applymovement LOCALID_CHERRYGROVE_RIVAL, PolishedReturn_RivalStarts
	waitmovement 0
	applymovement LOCALID_CHERRYGROVE_RIVAL, PolishedIntro_Exclamation
	waitmovement 0
	applymovement LOCALID_CHERRYGROVE_RIVAL, PolishedReturn_RivalBack
	waitmovement 0
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	msgbox PolishedReturn_CherrygroveRivalTextAfter2, MSGBOX_DEFAULT
	closemessage
	turnobject OBJ_EVENT_ID_PLAYER, DIR_WEST
	applymovement LOCALID_CHERRYGROVE_RIVAL, PolishedReturn_RivalExit
	waitmovement 0
	removeobject LOCALID_CHERRYGROVE_RIVAL
	setflag FLAG_TEMP_2
	special HealPlayerParty
	setflag FLAG_POLISHED_CHERRYGROVE_RIVAL_DONE
	setvar VAR_POLISHED_CHERRYGROVE_SCENE, 2
	releaseall
	end

'''
    for source, target in (('RivalWalksToYou','RivalApproach'), ('RivalPushesYouOutOfTheWay','PlayerPush'), ('RivalStartsToLeave','RivalStarts'), ('RivalComesBack','RivalBack'), ('RivalExitsStageLeft','RivalExit')):
        body = re.search(r'^CherrygroveCity_'+source+r':\n(.*?)^\s*step_end', script, re.M|re.S)[1]
        commands = []
        for line in body.splitlines():
            line = line.strip()
            if not line or line.endswith(':'): continue
            assert re.fullmatch(r'(run_step|step|turn_head)_(up|down|left|right)', line)
            commands.append(line.replace('run_step_','walk_fast_').replace('turn_head_','face_').replace('step_','walk_'))
        code += 'PolishedReturn_'+target+':\n'+''.join('\t'+c+'\n' for c in commands)+'\tstep_end\n\n'
    for label in ('CherrygroveRivalText_Seen','RivalCherrygroveWinText','RivalCherrygroveLossText','CherrygroveRivalTextAfter1','CherrygroveRivalTextAfter2'):
        code += define('PolishedReturn_'+label, text(script,label))
    return '\tmap_script MAP_SCRIPT_ON_LOAD, PolishedReturn_CityLoad\n', code


def lab(script, header, text, define):
    officer = obj(script, 'CopScript', 'LOCALID_ELMS_LAB_OFFICER', 'OFFICER', 'FLAG_TEMP_6', 'UP')
    officer['script'] = 'PolishedReturn_Cop'
    header['object_events'].append(officer)
    for x, label in ((4,'Left'),(5,'Right')):
        coord(header,x,5,'VAR_POLISHED_ELMS_LAB_SCENE',3,'PolishedReturn_Cop'+label)
    assert re.search(r'bg_event\s+5,\s*0, BGEVENT_READ, ElmsLabWindow',script)
    header['bg_events'].append({'type':'sign','x':5,'y':0,'elevation':0,
        'player_facing_dir':'BG_EVENT_PLAYER_FACING_ANY','script':'PolishedReturn_LabWindow'})
    code = '''PolishedReturn_LabVisibility::
	setflag FLAG_TEMP_6
	goto_if_ne VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedReturn_LabVisibilityDone
	goto_if_set FLAG_POLISHED_ELM_THEFT_REPORTED, PolishedReturn_LabVisibilityDone
	clearflag FLAG_TEMP_6
PolishedReturn_LabVisibilityDone::
	return
PolishedReturn_RemoveOfficer::
	removeobject LOCALID_ELMS_LAB_OFFICER
	return
PolishedReturn_LabWindow::
	goto_if_eq VAR_POLISHED_ERRAND_STATE, 2, PolishedReturn_BrokenWindow
	msgbox PolishedReturn_ElmsLabWindowText1, MSGBOX_SIGN
	end
PolishedReturn_BrokenWindow::
	msgbox PolishedReturn_ElmsLabWindowText2, MSGBOX_SIGN
	end
PolishedReturn_CopRight::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, PolishedReturn_PlayerLeft
	waitmovement 0
	goto PolishedReturn_CopApproach
PolishedReturn_CopLeft::
	lockall
PolishedReturn_CopApproach::
	applymovement OBJ_EVENT_ID_PLAYER, PolishedReturn_WalkToCop
	waitmovement 0
PolishedReturn_Cop::
	lockall
	turnobject LOCALID_ELMS_LAB_OFFICER, DIR_WEST
	msgbox PolishedReturn_ElmsLabOfficerText1, MSGBOX_DEFAULT
	closemessage
	removeobject LOCALID_ELMS_LAB_LYRA
	setobjectxyperm LOCALID_ELMS_LAB_LYRA, 5, 8
	clearflag FLAG_TEMP_5
	addobject LOCALID_ELMS_LAB_LYRA
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedReturn_LyraRunsIn
	waitmovement 0
	turnobject LOCALID_ELMS_LAB_OFFICER, DIR_SOUTH
	msgbox PolishedReturn_ElmsLabLyraTheftInnocentText, MSGBOX_DEFAULT
	delay 10
	turnobject LOCALID_ELMS_LAB_OFFICER, DIR_WEST
	msgbox PolishedReturn_ElmsLabOfficerText2, MSGBOX_DEFAULT
	closemessage
	fadescreen FADE_TO_BLACK
	special PolishedErrandNameRival
	waitstate
	special PolishedErrandBufferRivalName
	msgbox PolishedReturn_ElmsLabOfficerText3, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedReturn_LyraAside
	waitmovement 0
	applymovement LOCALID_ELMS_LAB_OFFICER, PolishedReturn_OfficerLeaves
	waitmovement 0
	removeobject LOCALID_ELMS_LAB_OFFICER
	setflag FLAG_TEMP_6
	delay 10
	turnobject LOCALID_ELMS_LAB_LYRA, DIR_NORTH
	turnobject OBJ_EVENT_ID_PLAYER, DIR_SOUTH
	msgbox PolishedReturn_ElmsLabLyraTheftGoodbyeText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedReturn_LyraLeaves
	waitmovement 0
	removeobject LOCALID_ELMS_LAB_LYRA
	setflag FLAG_TEMP_5
	setflag FLAG_POLISHED_ELM_THEFT_REPORTED
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 2
	delay 10
	applymovement OBJ_EVENT_ID_PLAYER, PolishedReturn_GiveEggApproach
	waitmovement 0
	goto PolishedReturn_GiveEgg

PolishedReturn_ElmTalk::
	lockall
	faceplayer
	goto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_ElmStudying
PolishedReturn_GiveEgg::
	msgbox PolishedReturn_ElmAfterTheftText1, MSGBOX_DEFAULT
	checkitem ITEM_POLISHED_MYSTERY_EGG
	goto_if_eq VAR_RESULT, FALSE, PolishedReturn_ElmDone
	msgbox PolishedReturn_ElmAfterTheftText2, MSGBOX_DEFAULT
	removeitem ITEM_POLISHED_MYSTERY_EGG, 1
	closemessage
	getplayerxy VAR_0x8004, VAR_0x8005
	call PolishedReturn_ElmRecoil
	msgbox PolishedReturn_ElmAfterTheftText3, MSGBOX_DEFAULT
	closemessage
	call PolishedReturn_ElmRecover
	msgbox PolishedReturn_ElmAfterTheftText4, MSGBOX_DEFAULT
	msgbox PolishedReturn_ElmAfterTheftText5, MSGBOX_DEFAULT
	setflag FLAG_POLISHED_GAVE_MYSTERY_EGG
	msgbox PolishedReturn_ElmAfterTheftText6, MSGBOX_DEFAULT
PolishedReturn_ElmDone::
	releaseall
	end
PolishedReturn_ElmStudying::
	msgbox PolishedReturn_ElmStudyingEggText, MSGBOX_DEFAULT
	releaseall
	end
PolishedReturn_Aide::
	msgbox PolishedReturn_AideText_TheftTestimony, MSGBOX_NPC
	end

PolishedReturn_ElmRecoil::
	goto_if_eq VAR_0x8004, 4, PolishedReturn_ElmRight
	goto_if_eq VAR_0x8004, 6, PolishedReturn_ElmLeft
	goto_if_eq VAR_0x8005, 1, PolishedReturn_ElmDown
	goto PolishedReturn_ElmUp
PolishedReturn_ElmRecover::
	goto_if_eq VAR_0x8004, 4, PolishedReturn_ElmLeft
	goto_if_eq VAR_0x8004, 6, PolishedReturn_ElmRight
	goto_if_eq VAR_0x8005, 1, PolishedReturn_ElmUp
	goto PolishedReturn_ElmDown
'''
    for d in ('Up','Down','Left','Right'):
        code += f'PolishedReturn_Elm{d}::\n\tapplymovement LOCALID_ELMS_LAB_ELM, PolishedReturn_ElmStep{d}\n\twaitmovement 0\n\treturn\nPolishedReturn_ElmStep{d}:\n\tlock_facing_direction\n\twalk_fast_{d.lower()}\n\tunlock_facing_direction\n\tstep_end\n\n'
    code += 'PolishedReturn_PlayerLeft:\n\twalk_left\n\tstep_end\n\n'
    for source,target in (('MeetCopScript_WalkUp','WalkToCop'),('LyraRunsInMovement','LyraRunsIn'),('LyraStepsAsideMovement','LyraAside'),('OfficerLeavesMovement','OfficerLeaves'),('LyraLeavesMovement','LyraLeaves'),('MeetCopScript_GiveEgg','GiveEggApproach')):
        code += movement(script,source,'PolishedReturn_'+target)
    for label in ('ElmsLabOfficerText1','ElmsLabOfficerText2','ElmsLabOfficerText3','ElmsLabLyraTheftInnocentText','ElmsLabLyraTheftGoodbyeText',*(f'ElmAfterTheftText{i}' for i in range(1,7)),'ElmStudyingEggText','AideText_TheftTestimony','ElmsLabWindowText1','ElmsLabWindowText2'):
        code += define('PolishedReturn_'+label,text(script.replace('<RIVAL>','{STR_VAR_1}'),label))
    return code


def adapt(name, script, header, files, code, text, define):
    if name == 'NewBarkTown':
        code=code.replace('PolishedNewBarkTown_ResidentSchedule::\n','PolishedNewBarkTown_ResidentSchedule::\n\tclearflag FLAG_POLISHED_MOM_FIRST_VISIT\n')
        code=code.replace('\tmsgbox PolishedNewBarkTown_Breeze, MSGBOX_DEFAULT\n','''	goto_if_set FLAG_POLISHED_MOM_BANK_OPEN, PolishedReturn_TeacherAfterMom
	goto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_TeacherAfterEgg
	goto_if_ne VAR_POLISHED_STARTER, 0, PolishedReturn_TeacherAfterStarter
	msgbox PolishedNewBarkTown_Breeze, MSGBOX_DEFAULT
	goto PolishedReturn_TeacherTravel
PolishedReturn_TeacherAfterMom::
	msgbox PolishedReturn_Text_CallMomOnGear, MSGBOX_DEFAULT
	goto PolishedReturn_TeacherTravel
PolishedReturn_TeacherAfterEgg::
	msgbox PolishedReturn_Text_TellMomIfLeaving, MSGBOX_DEFAULT
	goto PolishedReturn_TeacherTravel
PolishedReturn_TeacherAfterStarter::
	msgbox PolishedReturn_Text_YourMonIsAdorable, MSGBOX_DEFAULT
PolishedReturn_TeacherTravel::
''')
        for s in ('Text_TellMomIfLeaving','Text_YourMonIsAdorable'):
            code+=define('PolishedReturn_'+s,text(script,s))
        # Direct the player to the live home interaction until outbound phone calls exist.
        advice=text(script,'Text_CallMomOnGear')
        assert advice.startswith('Call your mom on\\nyour POKégear to')
        code+=define('PolishedReturn_Text_CallMomOnGear',advice.replace('Call your mom on\\nyour POKégear to','Visit your mom to'))
    elif name == 'CherrygroveCity':
        hooks, extra = city(script,header,text,define)
        code = code.replace('\t.byte 0\n',hooks+'\t.byte 0\n',1)+extra
        code = code.replace('PolishedCherrygroveCity_CherrygroveYoungsterScript::\n', 'PolishedCherrygroveCity_CherrygroveYoungsterScript::\n\tgoto_if_set FLAG_SYS_POKEDEX_GET, PolishedReturn_CityYoungster\n')
        code += 'PolishedReturn_CityYoungster::\n\tmsgbox PolishedReturn_CityYoungsterText, MSGBOX_NPC\n\tend\n'+define('PolishedReturn_CityYoungsterText',text(script,'CherrygroveYoungsterText_HavePokedex'))
    elif name == 'MrPokemonsHouse':
        code = code.replace('\tsetvar VAR_POLISHED_ERRAND_STATE, 1\n','\tsetvar VAR_POLISHED_ERRAND_STATE, 1\n\tsetvar VAR_POLISHED_CHERRYGROVE_SCENE, 1\n\tsetvar VAR_POLISHED_ELMS_LAB_SCENE, 3\n')
        code = code.replace('PolishedMrPokemonsHouse_Talk::\n','PolishedMrPokemonsHouse_Talk::\n\tgoto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_MrPokemonAfter\n')
        code += 'PolishedReturn_MrPokemonAfter::\n\tmsgbox PolishedReturn_MrPokemonAfterText, MSGBOX_NPC\n\tend\n'+define('PolishedReturn_MrPokemonAfterText',text(script,'MrPokemonText_AlwaysNewDiscoveries'))
    elif name == 'Route30':
        for o in header['object_events']:
            if o['local_id'] in ('LOCALID_ROUTE30_JOEY','LOCALID_ROUTE30_RATTATA','LOCALID_ROUTE30_PIDGEY'):
                o['flag'] = 'FLAG_POLISHED_GAVE_MYSTERY_EGG'
        joey = object_at('LOCALID_ROUTE30_JOEY_TRAINER','POLISHED_YOUNGSTER_BLUE',2,28,'PolishedReturn_Joey','FLAG_TEMP_2')
        joey.update(movement_type='MOVEMENT_TYPE_FACE_RIGHT',trainer_type='TRAINER_TYPE_NORMAL',trainer_sight_or_berry_tree_id='3')
        header['object_events'].append(joey)
        code = code.replace('\t.byte 0\n','\tmap_script MAP_SCRIPT_ON_FRAME_TABLE, PolishedReturn_PhoneFrame\n\t.byte 0\n',1)
        code = code.replace('PolishedRoute30_Setup::\n','PolishedRoute30_Setup::\n\tsetflag FLAG_TEMP_2\n\tcall_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_ShowJoey\n')
        code = code.replace('PolishedRoute30_Youngster::\n','PolishedRoute30_Youngster::\n\tgoto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_RouteYoungster\n')
        code += 'PolishedReturn_RouteYoungster::\n\tmsgbox PolishedReturn_RouteYoungsterText, MSGBOX_NPC\n\tend\n'+define('PolishedReturn_RouteYoungsterText',text(script,'Route30YoungsterText_EveryoneIsBattling'))
        code += '''PolishedReturn_ShowJoey::
	clearflag FLAG_TEMP_2
	return
PolishedReturn_PhoneFrame::
	map_script_2 VAR_POLISHED_ERRAND_STATE, 1, PolishedReturn_ElmCall
	.2byte 0
PolishedReturn_ElmCall::
	lockall
	playse SE_PC_LOGIN
	msgbox PolishedReturn_PhoneCaller, MSGBOX_DEFAULT
	msgbox PolishedReturn_ElmPhoneDisasterText, MSGBOX_DEFAULT
	setvar VAR_POLISHED_ERRAND_STATE, 2
	releaseall
	end
PolishedReturn_PhoneCaller:
	.string "Incoming call: PROF. ELM$"
PolishedReturn_Joey::
	trainerbattle_single TRAINER_POLISHED_JOEY, PolishedReturn_JoeySeen, PolishedReturn_JoeyBeaten
	msgbox PolishedReturn_JoeyAfter, MSGBOX_AUTOCLOSE
	end
'''
        assert 'SpecialCallOnlyWhenOutside, PHONE_ELM,  ElmPhoneScript2' in files['data/phone/special_calls.asm'].decode()
        assert '.disaster\n\tfarwritetext ElmPhoneDisasterText\n\tspecialphonecall SPECIALCALL_NONE\n\tsetevent EVENT_ELM_CALLED_ABOUT_STOLEN_POKEMON' in files['engine/phone/scripts/elm.asm'].decode()
        code += define('PolishedReturn_ElmPhoneDisasterText',text(files['data/phone/text/elm.asm'].decode(),'ElmPhoneDisasterText'))
        for s in ('Seen','Beaten','After'):
            code += define('PolishedReturn_Joey'+s,text(script,'YoungsterJoey1'+s+'Text'))
    elif name == 'ElmsLab':
        assert 'rawchar "Silver@"' in files['engine/events/specials.asm'].decode().split('SpecialNameRival:')[1].split('SpecialTrendyPhrase:')[0]
        code = code.replace('PolishedIntro_LabVisibility::\n','PolishedIntro_LabVisibility::\n\tcall PolishedReturn_LabVisibility\n')
        code = code.replace('\tcall_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 5, PolishedBattle_HideLyra\n','\tcall_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 5, PolishedBattle_HideLyra\n\tcall_if_eq VAR_POLISHED_ELMS_LAB_SCENE, 3, PolishedBattle_HideLyra\n')
        code = code.replace('PolishedIntro_LabRestore::\n','PolishedIntro_LabRestore::\n\tcall_if_set FLAG_TEMP_6, PolishedReturn_RemoveOfficer\n')
        code = code.replace('PolishedIntro_ProfElmScript::\n','PolishedIntro_ProfElmScript::\n\tgoto_if_eq VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedReturn_ElmTalk\n')
        code = code.replace('PolishedIntro_ElmsAideScript::\n','PolishedIntro_ElmsAideScript::\n\tgoto_if_set FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedReturn_AideBusy\n\tgoto_if_eq VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedReturn_Aide\nPolishedReturn_AideBusy::\n')
        code += lab(script,header,text,define)
    return code
