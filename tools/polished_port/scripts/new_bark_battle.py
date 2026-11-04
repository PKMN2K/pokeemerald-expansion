"""Pinned Lyra battle/portrait and the aide's first Potion, using native events."""
import hashlib
import json
import re
import tempfile
from pathlib import Path

from build_johto_demo import FIXTURES, PIN
from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png
from new_bark_intro import coord, movement


def battle_assets(files):
    source = json.loads((FIXTURES / 'new_bark/lyra_battle_excerpts.json').read_text())
    assert source['commit'] == PIN
    for entry in source['extracts']:
        assert hashlib.sha256(entry['excerpt'].encode()).hexdigest() == entry['sha256']
    parties, *learnsets, dvs = [entry['excerpt'] for entry in source['extracts']]
    assert re.search(r'db\s+0, \$BB, \$BB, \$BB, FEMALE ; lyra1', dvs)
    code = '/* Generated from the pinned Lyra1 opening parties and Lv5 learnsets. */\n'
    for index, nickname, species in re.findall(r'def_trainer LYRA1_(\d), "Lyra"\s+tr_mon 5, "(\w+)", (\w+)\s+end_trainer', parties):
        learnset = next(text for text in learnsets if f'evos_attacks {species.title()}\n' in text)
        moves = re.findall(r'learnset (\d+), (\w+)', learnset)
        assert len(moves) == 2 and all(int(level) <= 5 for level, _ in moves)
        code += f'''\n=== TRAINER_POLISHED_LYRA_{species} ===
Name: Lyra
Class: Rival
Pic: Polished Lyra
Gender: Female
Music: Female
Double Battle: No

{nickname} ({species.title()}) (F)
Level: 5
IVs: 22 HP / 22 Atk / 22 Def / 22 SpA / 22 SpD / 22 Spe
'''
        # Gen II stat contribution 2*DV is represented by a native Gen III IV.
        code += ''.join('- ' + move.replace('_', ' ').title() + '\n' for _, move in moves)
    assert code.count('=== TRAINER_') == 3
    products = {'src/data/polished_lyra.party': code.encode()}
    with tempfile.TemporaryDirectory() as tmp:
        src, out = Path(tmp) / 'source.png', Path(tmp) / 'out.png'
        src.write_bytes(files['gfx/trainers/lyra1.png'])
        w, h, pixels = _read_png_2bit_grayscale(src)
        assert (w, h) == (56, 56)
        canvas = [[0] * 64 for _ in range(64)]
        for y in range(h):
            for x in range(w):
                canvas[y + 8][x + 4] = 3 - pixels[y][x]
        _write_indexed_4bit_png(out, canvas)
        products['graphics/trainers/front_pics/polished_lyra.png'] = out.read_bytes()
    colors = [tuple(map(int, rgb)) for rgb in re.findall(r'RGB\s+(\d+),\s*(\d+),\s*(\d+)', files['gfx/trainers/lyra1.pal'].decode())]
    assert len(colors) == 2
    colors = [(31, 31, 31), *colors, (0, 0, 0)] + [(0, 0, 0)] * 12
    # JASC -> RGB555 truncates the low three bits; bit replication preserves
    # each original five-bit channel exactly.
    pal = 'JASC-PAL\n0100\n16\n' + ''.join(' '.join(str((c << 3) | (c >> 2)) for c in color) + '\n' for color in colors)
    products['graphics/trainers/palettes/polished_lyra.pal'] = pal.encode()
    return products


def lab(script, header, source_text, text_definition):
    for x, y, scene, source, target in (
        (4, 6, 6, 'LyraBattleScript', 'PolishedBattle_Lyra'),
        (4, 8, 5, 'AideScript_WalkPotions1', 'PolishedBattle_AideLeft'),
        (5, 8, 5, 'AideScript_WalkPotions2', 'PolishedBattle_AideRight'),
    ):
        assert re.search(rf'coord_event\s+{x},\s+{y}, {scene}, {source}', script)
        coord(header, x, y, 'VAR_POLISHED_ELMS_LAB_SCENE', scene, target)
    lyra = next(obj for obj in header['object_events'] if obj['local_id'] == 'LOCALID_ELMS_LAB_LYRA')
    lyra['flag'] = 'FLAG_TEMP_5'
    code = '''@ Scene 6: both battle outcomes continue in the lab, never white out.
PolishedBattle_Lyra::
	lockall
	turnobject LOCALID_ELMS_LAB_LYRA, DIR_SOUTH
	msgbox PolishedBattle_ElmsLabLyraChallengeText, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedBattle_Approach
	waitmovement 0
	turnobject LOCALID_PLAYER, DIR_EAST
	setvar VAR_LAST_TALKED, LOCALID_ELMS_LAB_LYRA
	goto_if_eq VAR_POLISHED_STARTER, 2, PolishedBattle_Cyndaquil
	goto_if_eq VAR_POLISHED_STARTER, 3, PolishedBattle_Totodile
	trainerbattle_earlyrival TRAINER_POLISHED_LYRA_CHIKORITA, RIVAL_BATTLE_HEAL_AFTER, PolishedBattle_ElmsLabLyraWinText, PolishedBattle_ElmsLabLyraLossText
	goto PolishedBattle_After
PolishedBattle_Cyndaquil::
	trainerbattle_earlyrival TRAINER_POLISHED_LYRA_CYNDAQUIL, RIVAL_BATTLE_HEAL_AFTER, PolishedBattle_ElmsLabLyraWinText, PolishedBattle_ElmsLabLyraLossText
	goto PolishedBattle_After
PolishedBattle_Totodile::
	trainerbattle_earlyrival TRAINER_POLISHED_LYRA_TOTODILE, RIVAL_BATTLE_HEAL_AFTER, PolishedBattle_ElmsLabLyraWinText, PolishedBattle_ElmsLabLyraLossText
PolishedBattle_After::
	lockall
	msgbox PolishedBattle_ElmsLabLyraText_YouLost, MSGBOX_DEFAULT
	turnobject LOCALID_ELMS_LAB_LYRA, DIR_NORTH
	msgbox PolishedBattle_ElmsLabLyraThankYouText, MSGBOX_DEFAULT
	turnobject LOCALID_ELMS_LAB_LYRA, DIR_WEST
	msgbox PolishedBattle_ElmsLabLyraSeeYouText, MSGBOX_DEFAULT
	closemessage
	turnobject LOCALID_PLAYER, DIR_SOUTH
	applymovement LOCALID_ELMS_LAB_LYRA, PolishedBattle_Leaves
	waitmovement 0
	removeobject LOCALID_ELMS_LAB_LYRA
	special HealPlayerParty
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 5
	releaseall
	end

@ Scene 5: a single Potion, from either source approach. Do not commit the
@ gift if native Bag insertion fails; walking across the trigger can retry.
PolishedBattle_AideLeft::
	lockall
	applymovement LOCALID_ELMS_LAB_AIDE, PolishedBattle_AideRight1
	waitmovement 0
	call PolishedBattle_Potion
	applymovement LOCALID_ELMS_LAB_AIDE, PolishedBattle_AideLeft1
	waitmovement 0
	releaseall
	end
PolishedBattle_AideRight::
	lockall
	applymovement LOCALID_ELMS_LAB_AIDE, PolishedBattle_AideRight2
	waitmovement 0
	call PolishedBattle_Potion
	applymovement LOCALID_ELMS_LAB_AIDE, PolishedBattle_AideLeft2
	waitmovement 0
	releaseall
	end
PolishedBattle_Potion::
	turnobject LOCALID_PLAYER, DIR_SOUTH
	msgbox PolishedBattle_AideText_GiveYouPotions, MSGBOX_DEFAULT
	giveitem ITEM_POTION
	goto_if_eq VAR_RESULT, FALSE, PolishedBattle_PotionDone
	setvar VAR_POLISHED_ELMS_LAB_SCENE, 2
PolishedBattle_PotionDone::
	msgbox PolishedIntro_AideText_AlwaysBusy, MSGBOX_DEFAULT
	closemessage
	return

PolishedBattle_RemoveLyra::
	removeobject LOCALID_ELMS_LAB_LYRA
	return
PolishedBattle_HideLyra::
	setflag FLAG_TEMP_5
	return

'''
    for source, target in (('LyraBattleMovement', 'Approach'), ('LyraLeavesMovement', 'Leaves'),
        ('AideWalksRight1', 'AideRight1'), ('AideWalksRight2', 'AideRight2'),
        ('AideWalksLeft1', 'AideLeft1'), ('AideWalksLeft2', 'AideLeft2')):
        code += movement(script, source, 'PolishedBattle_' + target)
    # Both post-battle outcome labels alias the same source text in v3.2.3.
    assert 'ElmsLabLyraText_YouWon:\nElmsLabLyraText_YouLost:' in script
    for label in ('ElmsLabLyraChallengeText', 'ElmsLabLyraWinText', 'ElmsLabLyraLossText',
                  'ElmsLabLyraText_YouLost', 'ElmsLabLyraThankYouText', 'ElmsLabLyraSeeYouText', 'AideText_GiveYouPotions'):
        code += text_definition('PolishedBattle_' + label, source_text(script, label))
    assert re.search(r'verbosegiveitem POTION\s+setscene \$2', script)
    return code, {'source_script': 'LyraBattleScript', 'trigger': [4, 6],
        'scenes': [6, 5, 2], 'can_lose': True, 'heal_party': True,
        'aide_triggers': [[4, 8], [5, 8]], 'gift': {'item': 'POTION', 'quantity': 1},
        'next_phase': "Mom's events and Route 29"}
