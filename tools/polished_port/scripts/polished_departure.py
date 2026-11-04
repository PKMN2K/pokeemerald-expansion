"""Mom's post-errand savings, Lyra's demonstration and Tuesday's Tuscany."""
import hashlib
import json
import re
import tempfile
from pathlib import Path

from build_johto_demo import FIXTURES, PIN
from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png
from new_bark_intro import coord


def assets(files):
    excerpts = json.loads((FIXTURES / 'new_bark/post_egg_excerpts.json').read_text())
    assert excerpts['commit'] == PIN
    for row in excerpts['extracts']:
        assert hashlib.sha256(row['text'].encode()).hexdigest() == row['sha256']
    assert 'Lyra1Palette' in excerpts['extracts'][2]['text']
    assert 'db "Lyra@"' in files['engine/events/catch_tutorial.asm'].decode()
    assert 'POKE_BALL, 10, PREMIER_BALL, 1' in files['engine/events/catch_tutorial.asm'].decode()
    with tempfile.TemporaryDirectory() as tmp:
        source, output = Path(tmp)/'back.png', Path(tmp)/'native.png'
        source.write_bytes(files['gfx/battle/lyra_back.png'])
        width, height, pixels = _read_png_2bit_grayscale(source)
        assert (width, height) == (48, 48)
        canvas = [[0]*64 for _ in range(64)]
        for y, row in enumerate(pixels):
            canvas[y+16][8:56] = [3-pixel for pixel in row]
        _write_indexed_4bit_png(output, canvas)
        return {'graphics/trainers/back_pics/polished_lyra.png':output.read_bytes()}


def bank(script, files, text, define):
    source = files['engine/events/mom.asm'].decode()
    assert 'db "Get@"\n\tdb "Save@"\n\tdb "Change@"\n\tdb "Cancel@"' in source
    common = re.sub(r'^SECTION .*\n', '', files['data/text/common.asm'].decode(), flags=re.M)
    common = re.sub(r'^(\s*)prompt\s*$',r'\1done',common,flags=re.M)
    code = """PolishedMom_FirstBank::
    setflag FLAG_POLISHED_MOM_FIRST_VISIT
    msgbox PolishedMom_LeavingText1, MSGBOX_YESNO
    clearflag FLAG_POLISHED_MOM_SAVES_MONEY
    goto_if_eq VAR_RESULT, NO, PolishedMom_Opened
    setflag FLAG_POLISHED_MOM_SAVES_MONEY
    msgbox PolishedMom_LeavingText2, MSGBOX_DEFAULT
PolishedMom_Opened::
    setflag FLAG_POLISHED_MOM_BANK_OPEN
    msgbox PolishedMom_LeavingText3, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_FirstVisitAgain::
    msgbox PolishedMom_DoItText, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_Bank::
    msgbox PolishedMom_IsThisAboutYourMoneyText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_DSTCheck
    msgbox PolishedMom_BankWhatDoYouWantToDoText, MSGBOX_DEFAULT
    multichoice 0, 0, MULTI_POLISHED_MOM_BANK, FALSE
    goto_if_eq VAR_RESULT, 0, PolishedMom_Withdraw
    goto_if_eq VAR_RESULT, 1, PolishedMom_Deposit
    goto_if_eq VAR_RESULT, 2, PolishedMom_Change
    goto PolishedMom_Cancel
PolishedMom_Withdraw::
    setvar VAR_0x8004, 0
    msgbox PolishedMom_TakeMoneyText, MSGBOX_DEFAULT
    goto PolishedMom_Transfer
PolishedMom_Deposit::
    setvar VAR_0x8004, 1
    msgbox PolishedMom_StoreMoneyText, MSGBOX_DEFAULT
PolishedMom_Transfer::
    closemessage
    specialvar VAR_RESULT, PolishedMomOpenTransfer
    goto_if_eq VAR_RESULT, FALSE, PolishedMom_Cancel
    waitstate
    goto_if_eq VAR_RESULT, 0, PolishedMom_Cancel
    goto_if_eq VAR_RESULT, 2, PolishedMom_Insufficient
    goto_if_eq VAR_RESULT, 3, PolishedMom_NoRoom
    playse SE_SHOP
    goto_if_eq VAR_0x8004, 0, PolishedMom_Taken
    msgbox PolishedMom_StoredMoneyText, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_Taken::
    msgbox PolishedMom_TakenMoneyText, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_Insufficient::
    goto_if_eq VAR_0x8004, 0, PolishedMom_InsufficientBank
    msgbox PolishedMom_InsufficientFundsInWalletText, MSGBOX_DEFAULT
    goto PolishedMom_Deposit
PolishedMom_InsufficientBank::
    msgbox PolishedMom_HaventSavedThatMuchText, MSGBOX_DEFAULT
    goto PolishedMom_Withdraw
PolishedMom_NoRoom::
    goto_if_eq VAR_0x8004, 0, PolishedMom_WalletFull
    msgbox PolishedMom_NotEnoughRoomInBankText, MSGBOX_DEFAULT
    goto PolishedMom_Deposit
PolishedMom_WalletFull::
    msgbox PolishedMom_NotEnoughRoomInWalletText, MSGBOX_DEFAULT
    goto PolishedMom_Withdraw
PolishedMom_Change::
    msgbox PolishedMom_SaveMoneyText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_Stop
    setflag FLAG_POLISHED_MOM_SAVES_MONEY
    msgbox PolishedMom_StartSavingMoneyText, MSGBOX_DEFAULT
    goto PolishedMom_Done
PolishedMom_Stop::
    clearflag FLAG_POLISHED_MOM_SAVES_MONEY
PolishedMom_Cancel::
    msgbox PolishedMom_JustDoWhatYouCanText, MSGBOX_DEFAULT
PolishedMom_Done::
    release
    end
PolishedMom_DSTCheck::
    gettime
    goto_if_set FLAG_POLISHED_DAYLIGHT_SAVING, PolishedMom_DSTBack
    goto_if_eq VAR_0x8000, 23, PolishedMom_DSTBoundary
    msgbox PolishedMom_AskDSTText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_Cancel
    specialvar VAR_RESULT, PolishedMomAdjustClock
    goto_if_eq VAR_RESULT, FALSE, PolishedMom_DSTLost
    msgbox PolishedMom_DSTText, MSGBOX_DEFAULT
    goto PolishedMom_Cancel
PolishedMom_DSTBack::
    goto_if_eq VAR_0x8000, 0, PolishedMom_DSTBoundary
    msgbox PolishedMom_AskNotDSTText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_Cancel
    specialvar VAR_RESULT, PolishedMomAdjustClock
    goto_if_eq VAR_RESULT, FALSE, PolishedMom_DSTLost
    msgbox PolishedMom_NotDSTText, MSGBOX_DEFAULT
    goto PolishedMom_Cancel
PolishedMom_DSTBoundary::
    msgbox PolishedMom_AskAdjustDSTText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedMom_Cancel
PolishedMom_DSTLost::
    msgbox PolishedMom_LostGearBookletText, MSGBOX_DEFAULT
    goto PolishedMom_Cancel
"""
    code += define('PolishedMom_DoItText', text(script,'MomDoItText'))
    labels = ('LeavingText1','LeavingText2','LeavingText3','IsThisAboutYourMoneyText','BankWhatDoYouWantToDoText',
              'TakeMoneyText','StoreMoneyText','SaveMoneyText','HaventSavedThatMuchText','NotEnoughRoomInWalletText',
              'InsufficientFundsInWalletText','NotEnoughRoomInBankText','StartSavingMoneyText','StoredMoneyText',
              'TakenMoneyText','JustDoWhatYouCanText','LostGearBookletText')
    for name in labels:
        code += define('PolishedMom_'+name, text(common,'_Mom'+name))
    for name in ('AskDSTText','DSTText','AskNotDSTText','NotDSTText','AskAdjustDSTText'):
        code += define('PolishedMom_'+name,text(common,'_Timeset'+name))
    return code


def route(script, header, text, define, object_at):
    lyra=object_at('LOCALID_ROUTE29_LYRA','POLISHED_LYRA',50,12,'PolishedDeparture_Silent','FLAG_TEMP_2')
    tuscany=object_at('LOCALID_ROUTE29_TUSCANY','POLISHED_POKEFAN_PURPLE',29,12,'PolishedDeparture_Tuscany','FLAG_TEMP_3')
    for npc, source in ((lyra,'LYRA'),(tuscany,'POKEFAN_F')):
        assert re.search(rf"object_event\s+{npc['x']},\s*{npc['y']}, SPRITE_{source}, SPRITEMOVEDATA_SPINRANDOM_SLOW",script)
        npc['movement_type']='MOVEMENT_TYPE_LOOK_AROUND'
        header['object_events'].append(npc)
    code = """PolishedDeparture_RouteSetup::
    setvar VAR_TEMP_2, 0
    setflag FLAG_TEMP_2
    goto_if_unset FLAG_POLISHED_GAVE_MYSTERY_EGG, PolishedDeparture_Tuesday
    goto_if_set FLAG_POLISHED_CAUGHT_TUTORIAL, PolishedDeparture_Tuesday
    clearflag FLAG_TEMP_2
    setvar VAR_TEMP_2, 1
PolishedDeparture_Tuesday::
    setflag FLAG_TEMP_3
    goto_if_unset FLAG_POLISHED_MOM_BANK_OPEN, PolishedDeparture_MaskDone
    specialvar VAR_TEMP_A, PolishedJohtoGetWeekday
    goto_if_ne VAR_TEMP_A, 2, PolishedDeparture_MaskDone
    clearflag FLAG_TEMP_3
PolishedDeparture_MaskDone::
    return
PolishedDeparture_Silent::
    end
"""
    for index,y in enumerate((8,9),1):
        assert re.search(rf'coord_event 53,\s*{y}, 1, Route29Tutorial{index}',script)
        coord(header,53,y,'VAR_TEMP_2',1,f'PolishedDeparture_Approach{index}')
        code += f"""PolishedDeparture_Approach{index}::
    lockall
    checkitemspace ITEM_POKE_BALL, 5
    goto_if_eq VAR_RESULT, FALSE, PolishedDeparture_Full
    turnobject LOCALID_ROUTE29_LYRA, DIR_NORTH
    applymovement LOCALID_ROUTE29_LYRA, PolishedIntro_Exclamation
    waitmovement 0
    delay 15
    applymovement LOCALID_ROUTE29_LYRA, PolishedDeparture_Move{index}a
    waitmovement 0
    turnobject OBJ_EVENT_ID_PLAYER, DIR_WEST
    msgbox PolishedDeparture_CatchingTutorialIntroText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, PolishedDeparture_Decline
    closemessage
    applymovement OBJ_EVENT_ID_PLAYER, PolishedDeparture_Follow{index}
    applymovement LOCALID_ROUTE29_LYRA, PolishedDeparture_Move{index}b
    waitmovement OBJ_EVENT_ID_PLAYER
    waitmovement LOCALID_ROUTE29_LYRA
    goto PolishedDeparture_Demonstrate
"""
    code += """PolishedDeparture_Demonstrate::
    special SavePlayerParty
    special StartPolishedLyraTutorial
    waitstate
    special LoadPlayerParty
    turnobject LOCALID_ROUTE29_LYRA, DIR_NORTH
    msgbox PolishedDeparture_CatchingTutorialDebriefText, MSGBOX_DEFAULT
    goto PolishedDeparture_Gift
PolishedDeparture_Decline::
    setflag FLAG_POLISHED_DECLINED_TUTORIAL
    msgbox PolishedDeparture_CatchingTutorialRefusedText, MSGBOX_DEFAULT
PolishedDeparture_Gift::
    giveitem ITEM_POKE_BALL, 5
    msgbox PolishedDeparture_CatchingTutorialGoodbyeText, MSGBOX_DEFAULT
    closemessage
    applymovement LOCALID_ROUTE29_LYRA, PolishedDeparture_Move3
    waitmovement 0
    removeobject LOCALID_ROUTE29_LYRA
    setflag FLAG_TEMP_2
    setflag FLAG_POLISHED_CAUGHT_TUTORIAL
    setvar VAR_TEMP_2, 0
    releaseall
    end
PolishedDeparture_Full::
    msgbox PolishedDeparture_MakeRoom, MSGBOX_DEFAULT
    releaseall
    end
PolishedDeparture_Tuscany::
    lock
    faceplayer
    goto_if_set FLAG_POLISHED_TUSCANY_SILK_SCARF, PolishedDeparture_TuesdayAgain
    specialvar VAR_RESULT, PolishedJohtoGetWeekday
    goto_if_ne VAR_RESULT, 2, PolishedDeparture_WrongDay
    goto_if_set FLAG_POLISHED_MET_TUSCANY, PolishedDeparture_Scarf
    msgbox PolishedDeparture_MeetTuscanyText, MSGBOX_DEFAULT
    setflag FLAG_POLISHED_MET_TUSCANY
PolishedDeparture_Scarf::
    msgbox PolishedDeparture_TuscanyGivesGiftText, MSGBOX_DEFAULT
    giveitem ITEM_SILK_SCARF
    goto_if_eq VAR_RESULT, FALSE, PolishedDeparture_TuscanyDone
    setflag FLAG_POLISHED_TUSCANY_SILK_SCARF
    msgbox PolishedDeparture_TuscanyGaveGiftText, MSGBOX_DEFAULT
    goto PolishedDeparture_TuscanyDone
PolishedDeparture_TuesdayAgain::
    msgbox PolishedDeparture_TuscanyTuesdayText, MSGBOX_DEFAULT
    goto PolishedDeparture_TuscanyDone
PolishedDeparture_WrongDay::
    msgbox PolishedDeparture_TuscanyNotTuesdayText, MSGBOX_DEFAULT
PolishedDeparture_TuscanyDone::
    release
    end
"""
    for suffix in ('1a','2a','1b','2b','3'):
        # 1a falls through into 2a. With Crystal follow, the player walks
        # onto the leader's previous tile each time she moves.
        body = script.split('LyraMovementData'+suffix+':\n',1)[1].split('step_end',1)[0]
        steps = re.findall(r'^\s*step_(left|up|down|right)\s*$',body,re.M)
        assert steps
        code += 'PolishedDeparture_Move'+suffix+':\n'+''.join('    walk_'+d+'\n' for d in steps)+'    step_end\n'
        if suffix.endswith('b'):
            follow=['left',*steps[:-1]]
            code += 'PolishedDeparture_Follow'+suffix[0]+':\n'+''.join('    walk_'+d+'\n' for d in follow)+'    step_end\n'
    for name in ('CatchingTutorialIntroText','CatchingTutorialDebriefText','CatchingTutorialRefusedText','CatchingTutorialGoodbyeText',
                 'MeetTuscanyText','TuscanyGivesGiftText','TuscanyGaveGiftText','TuscanyTuesdayText','TuscanyNotTuesdayText'):
        code += define('PolishedDeparture_'+name,text(script,name))
    code += define('PolishedDeparture_MakeRoom','Make room for five\\nPOKé BALLS first.$')
    return code
