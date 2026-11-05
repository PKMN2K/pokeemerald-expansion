"""Follow Earl using normal controls; read the authentic chart in both Bags."""
from hashlib import sha256
import struct
from PIL import Image

from test_academy import CHART_FRAME_SHA256
from test_johto_demo_emulator import A,B,UP,DOWN,LEFT,RIGHT
from test_new_bark_emulator import walk,map_data,actors,resident_id,save_game,continue_game,close_resident_dialogue
from test_new_bark_intro_emulator import advance,choice_prompt,layout,script_active,task_active,text_matches,party
from test_new_bark_battle_emulator import controller,choose_cell
from test_cherrygrove_emulator import flag,map_id,interact,extra_layout,enter,leave

P = "PolishedEarlsPokemonAcademy_"


def chart_slot(game):
    sb1=game.u32(game.sym["gSaveBlock1Ptr"])
    k=game.u32(game.u32(game.sym["gSaveBlock2Ptr"])+layout()["encryption_key"])&0xffff
    d=extra_layout()
    content=game.read(sb1+d["key_items"],4*d["key_count"])
    slots=[(i,qty^k) for i,(item,qty) in enumerate(struct.iter_unpack("<HH",content)) if item==876]
    if not slots:
        return None
    assert len(slots)==1 and slots[0][1]==1,slots
    return slots[0][0]


def wait_for(game, condition, description):
    for _ in range(260):
        if condition():
            return
        game.frames(10)
    raise AssertionError((description,game.map_number(),game.position()))


def cb(game,name):
    return game.u32(game.sym["gMain"]+4)==game.sym[name]|1


def verify_chart(game, index, key, shot, bag_open=False):
    if bag_open:
        game.frames(75)  # Newly created Bag tasks receive input after the fade.
        game.choose_bag_key_item(index)
    else:
        game.use_key_item(index)
    wait_for(game,lambda:cb(game,"CB2_PolishedTypeChart"),"open chart")
    game.frames(65);game.press(RIGHT,20)
    assert cb(game,"CB2_PolishedTypeChart")
    game.screenshot(shot)
    img=Image.open(game.output/(shot+".png")).convert("RGB").crop((40,8,200,152))
    rgb=bytearray()
    for r,g,b in img.getdata():
        rgb+=struct.pack("<H",(r>>3)|(g>>3)<<5|(b>>3)<<10)
    assert sha256(rgb).hexdigest()==CHART_FRAME_SHA256,("chart framebuffer",sha256(rgb).hexdigest())
    game.press(key,160)
    wait_for(game,lambda:task_active(game,"Task_BagMenu_HandleInput"),"return to same Bag")
    game.frames(65)
    assert game.read(game.sym["gBagPosition"]+5,1)==b"\x04"
    assert chart_slot(game)==index


def talk(game,direction,label):
    game.face(direction);game.press(A,40)
    advance(game,lambda:text_matches(game,label))


def tour(game,city,checks):
    approaches=(((13,17),UP),((12,16),RIGHT),((14,16),LEFT),((13,15),DOWN))
    game.snapshot("academy-before-tour")
    earl=resident_id(city,"LOCALID_VIOLET_EARL")
    for n,(pos,direction) in enumerate(approaches):
        game.restore("academy-before-tour");walk(game,city,pos)
        assert earl in actors(game) and actors(game)[earl]["position"]==(13,16)
        game.face(direction);game.press(A,35)
        choice_prompt(game,"PolishedEarl_Text_EarlAsksIfYouBeatFalkner")
        if n==0:
            game.screenshot("earl-outside")
            game.snapshot("academy-question")
            game.press(A,55)
            advance(game,lambda:text_matches(game,"PolishedEarl_Text_VeryNiceIndeed"))
            close_resident_dialogue(game)
            assert chart_slot(game) is None and not flag(game,0x4d6) and earl in actors(game)
            game.restore("academy-question")
        game.press(B,65)
        advance(game,lambda:text_matches(game,"PolishedEarl_Text_HereTeacherIAm"),limit=420)
        assert game.position()==(30,19) and actors(game)[earl]["position"]==(30,18),(n,game.position(),actors(game))
        game.screenshot("earl-school-door-"+str(n))
        advance(game,lambda:not script_active(game),limit=400)
        assert chart_slot(game) is not None and flag(game,0x4d6) and earl not in actors(game)
    save_game(game);continue_game(game)
    assert flag(game,0x4d6) and earl not in actors(game) and chart_slot(game) is not None
    checks.append("Earl accepts Yes without starting a tour; all four approaches on No reach the Academy doorstep, award one Type Chart and persist after Save/Continue")


def classroom(game,city,school,checks):
    enter(game,city,school)
    earl=resident_id(school,"LOCALID_ACADEMY_EARL")
    walk(game,school,(4,3))
    assert earl in actors(game)
    game.snapshot("academy-inside")
    for win, extra, raise_mon in ((True,True,True),(True,False,False),(False,True,False)):
        game.restore("academy-inside");game.face(UP);game.press(A,40)
        choice_prompt(game,P+"AcademyEarlIntroText");game.press(A if win else B,55)
        if win:
            choice_prompt(game,P+"AcademyEarlTeachHowToWinText");game.press(A if extra else B,55)
        if not win or extra:
            choice_prompt(game,P+"AcademyEarlTeachMoreText");game.press(A if raise_mon else B,55)
        expect="AcademyEarlTeachHowToRaiseWellText" if raise_mon else "AcademyEarlNoMoreToTeachText"
        advance(game,lambda:text_matches(game,P+expect));close_resident_dialogue(game)
        assert chart_slot(game) is not None
    for tile in ((3,1),(4,1)):
        walk(game,school,tile);talk(game,UP,P+"AcademyBlackboardText")
        advance(game,lambda:task_active(game,"Task_HandleMultichoiceGridInput"));game.frames(30)
        if tile==(3,1):
            game.screenshot("academy-blackboard")
            for i,subject in enumerate(("Poison","Paralysis","Sleep","Burn","Freeze","Quit")):
                for key in [DOWN]*(i//2)+[RIGHT]*(i%2):
                    game.press(key,15)
                game.press(A,50)
                if subject=="Quit":
                    break
                advance(game,lambda:text_matches(game,P+"Academy"+subject+"Text"))
                advance(game,lambda:task_active(game,"Task_HandleMultichoiceGridInput"));game.frames(30)
        else:
            game.press(B,55)
        advance(game,lambda:not script_active(game))
    for stop in range(4):
        walk(game,school,(1,4));game.face(RIGHT);game.press(A,40)
        for i in range(min(stop+1,3)):
            lab=P+"AcademyNotebookText"+(str(i) if i else "")
            choice_prompt(game,lab)
            game.press(B if i==stop else A,55)
        if stop==3:
            advance(game,lambda:text_matches(game,P+"AcademyNotebookText3"))
        advance(game,lambda:not script_active(game))
    interact(game,school,(0,2),UP,P+"JournalText");close_resident_dialogue(game)
    save_game(game);continue_game(game)
    assert flag(game,0x4d7) and flag(game,0x4d6) and chart_slot(game) is not None
    for pos,key,label in (((1,5),RIGHT,"EarlsPokemonAcademyYoungster1Text_Text"),((3,7),RIGHT,"EarlsPokemonAcademyYoungster2Text_Text"),
                          ((3,12),UP,"EarlsPokemonAcademyGameboyKid1Text"),((4,12),UP,"EarlsPokemonAcademyGameboyKid2Text")):
        interact(game,school,pos,key,P+label);close_resident_dialogue(game)
    game.screenshot("academy-classroom")
    for index in (0,1):
        game.restore("academy-inside");leave(game,school,city,index)
        assert game.position()==(30,18)
    checks.append("Earl’s lessons, all five blackboard entries, every notebook page and its decline paths, four students, Walker’s saved journal and both Academy exits work in mGBA")


def battle_chart(game,city,index,checks):
    walk(game,city,(24,21));walk(game,city,(21,10));game.face(DOWN);game.press(A,40)
    advance(game,lambda:bool(game.read(game.sym["gPlayerAvatar"],1)[0]&8) and not script_active(game))
    game.frames(150)
    for _ in range(500):
        if task_active(game,"Task_BattleStart") or not cb(game,"CB2_Overworld"):
            break
        start=game.position()
        assert start in ((21,11),(21,12)),start
        direction=DOWN if start[1]==11 else UP
        for _ in range(40):
            game.frames(1,direction)
            if game.position()!=start or task_active(game,"Task_BattleStart") or not cb(game,"CB2_Overworld"):
                break
        game.frames(35)
    advance(game,lambda:controller(game,"HandleInputChooseAction"),limit=800)
    before=(party(game,details=True),party(game,enemy=True,details=True),game.read(game.sym["gBattleTurnCounter"],1))
    choose_cell(game,"gActionSelectionCursor",1)
    wait_for(game,lambda:task_active(game,"Task_BagMenu_HandleInput"),"battle Bag")
    verify_chart(game,index,B,"earl-chart-battle",bag_open=True)
    game.press(B,180)
    wait_for(game,lambda:controller(game,"HandleInputChooseAction"),"return to battle command")
    assert before==(party(game,details=True),party(game,enemy=True,details=True),game.read(game.sym["gBattleTurnCounter"],1))
    assert chart_slot(game)==index
    checks.append("Reading the Type Chart in a real Violet pond battle returns to the same command menu with the turn, both parties and the Key Item preserved")


def check_academy(game,city,checks):
    school=map_data("PolishedEarlsPokemonAcademy")
    game.snapshot("academy-start")
    # The Academy is open before Earl moves there. The previous city save can
    # enter via its formerly solid door without leaving Violet to refresh it.
    enter(game,city,school)
    walk(game,school,(4,3))
    assert resident_id(school,"LOCALID_ACADEMY_EARL") not in actors(game)
    leave(game,school,city)
    game.restore("academy-start")
    tour(game,city,checks)
    index=chart_slot(game)
    verify_chart(game,index,A,"earl-chart-field-a")
    verify_chart(game,index,B,"earl-chart-field-b",bag_open=True)
    game.press(B,180)  # Bag to Start menu.
    game.press(B,60)   # Start menu to field.
    checks.append("The portable Type Chart reproduces all 23,040 source pixels on screen; A and B both return to its Key Items pocket")
    classroom(game,city,school,checks)
    battle_chart(game,city,index,checks)
    game.restore("academy-start")
    print("PASS: Earl’s tour, Academy lessons, field and battle charts",flush=True)
