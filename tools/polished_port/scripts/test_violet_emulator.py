"""Walk both Violet City gate rows, visit the Center and continue a cartridge save."""
from build_johto_demo import ROOT
from stage_johto_time_palettes import stage, load_source_palettes
from stage_johto_roof_palettes import load_roof_tables, roof_colors, apply_roof_colors
from test_johto_demo_emulator import A, B, UP, DOWN, LEFT, RIGHT
from test_new_bark_emulator import (actors, map_data, walk, resident_id, save_game,
                                    continue_game, close_resident_dialogue)
from test_new_bark_intro_emulator import advance, party, script_active, task_active, text_matches
from test_cherrygrove_emulator import enter as enter_building, leave, flag, interact, map_id


def check_violet(game, gate, checks):
    from test_route31_emulator import enter
    city, center = map_data("PolishedVioletCity"), map_data("PolishedVioletPokeCenter1F")
    # Both ace trainers patrol the only east-west opening. Plan around the
    # current actor, since reserving the whole patrol would seal the street.
    city = dict(city, object_events=[dict(obj, movement_range_x=0, movement_range_y=0)
                                     for obj in city["object_events"]])
    palettes = stage(load_source_palettes())[0]
    roofs = load_roof_tables()
    roof_tiles = (ROOT/"data/tilesets/primary/polished_violet_outdoor/tiles.bin").read_bytes()[11*32:20*32]
    game.snapshot("violet-before-gate")
    for index, (hour, period) in enumerate(((6,"morn"),(12,"day"),(18,"eve"),(23,"nite"))):
        game.restore("violet-before-gate")
        game.set_hour(hour)
        row = 4+index%2
        walk(game, gate, (1,row))
        enter(game, city, LEFT)
        assert game.position() == (39,row+20), (row,game.position())
        expected = apply_roof_colors(palettes["violet_ecruteak."+period], roof_colors(roofs,period,map_group=10)[0])
        for buffer in ("gPlttBufferUnfaded","gPlttBufferFaded"):
            assert game.read(game.sym[buffer],224)==expected, (period,buffer)
        bg2 = int.from_bytes(game.read(0x0400000c,2),"little")
        assert game.read(0x06000000+((bg2>>2)&3)*0x4000+11*32,len(roof_tiles))==roof_tiles, period
        assert flag(game,0x4cf)
        game.snapshot("violet-arrival")
        walk(game,city,(27,15))
        active=actors(game)
        for role,shown in (("M",hour<17),("F",hour>=17)):
            actor=resident_id(city,"LOCALID_VIOLET_CITY_ACE_TRAINER_"+role)
            assert (actor in active and not active[actor]["invisible"])==shown,(hour,role,active)
        game.screenshot("violet-"+period)
        if period=="day":
            game.snapshot("violet-day")
        game.restore("violet-arrival")
        enter(game,gate,RIGHT)
        assert game.position()==(0,row), (row,game.position())
    checks.append("Both Violet gate rows round-trip; Violet's roof tiles and all four RGB555 palettes match the source, and its day and evening residents switch on entry")

    game.restore("violet-day")
    interact(game,city,(24,21),UP,"PolishedVioletCity_VioletCitySignText_Text")
    close_resident_dialogue(game)
    enter_building(game,city,center)
    assert game.position()==(5,7),game.position()
    game.screenshot("violet-center")
    game.snapshot("violet-center")
    interact(game,center,(5,3),UP)
    advance(game,lambda:not script_active(game),limit=400)
    assert all(m["hp"]==m["max_hp"] and m["status"]==0 for m in party(game,details=True))
    interact(game,center,(10,2),UP,"PolishedVioletPokeCenter1F_JournalText")
    close_resident_dialogue(game)
    save_game(game);continue_game(game)
    assert game.map_number()==map_id(game,center) and flag(game,0x4d5) and flag(game,0x4cf)
    base = game.u32(game.sym["gSaveBlock1Ptr"])
    # Center entry sets the same persistent heal-location warp as its blackout destination.
    from test_cherrygrove_emulator import extra_layout
    heal = game.read(base+extra_layout()["heal"],8)
    assert tuple(heal[:2]) == map_id(game,center)
    checks.append("Violet Center heals the party; Falkner’s journal, city visit and blackout destination survive native Save/Continue")

    interact(game,center,(7,3),UP)
    advance(game,lambda:task_active(game,"Task_HandleMultichoiceInput"))
    game.press(A,160)
    advance(game,lambda:task_active(game,"Task_PCMainMenu"))
    game.press(A,160)
    advance(game,lambda:game.u32(game.sym["gMain"]+4)==game.sym["CB2_PokeStorage"]|1)
    game.frames(240);game.screenshot("violet-storage")
    for index in (0,1):
        game.restore("violet-center")
        leave(game,center,city,index)
        assert game.position()==(31,26), game.position()
    checks.append("The Center PC opens native Pokémon storage, and both exit tiles return to the original Violet City doorway")
    from test_route30_emulator import berries
    item = extra_layout()["cheri"]
    walk(game,city,(14,30))
    before=berries(game,item)
    game.face(UP);game.press(A,80)
    advance(game,lambda:any(text_matches(game,"PolishedRoute29_FruitObtained"+key)
                             for key in ("One","Two","Three")))
    close_resident_dialogue(game)
    after=berries(game,item)
    assert 1<=after-before<=3
    save_game(game);continue_game(game)
    assert berries(game,item)==after
    game.face(UP);game.press(A,80);close_resident_dialogue(game)
    assert berries(game,item)==after
    checks.append("The Cheri tree gives a real berry harvest and keeps its picked state after Save/Continue")

    from test_cherrygrove_emulator import sea_step
    game.restore("violet-center");leave(game,center,city)
    walk(game,city,(24,21))
    walk(game,city,(21,10));game.face(DOWN);game.press(A)
    advance(game,lambda:bool(game.read(game.sym["gPlayerAvatar"],1)[0]&8) and not script_active(game))
    game.frames(180)
    assert game.position()==(21,11),game.position()
    table = [(extra_layout()["poliwag"],15),(extra_layout()["poliwag"],20),
             (extra_layout()["poliwhirl"],20)]
    for _ in range(500):
        enemy = sea_step(game,DOWN if game.position()[1]==11 else UP,lose=True,water_table=table,area="violet")
        if enemy:
            break
    assert enemy is not None
    advance(game,lambda:not script_active(game),limit=400)
    assert game.read(game.sym["gBattleOutcome"],1)==b"\x02"
    assert game.map_number()==map_id(game,center) and game.position()==(5,3)
    assert all(m["hp"]==m["max_hp"] and m["status"]==0 for m in party(game,details=True))
    game.screenshot("violet-blackout")
    checks.append("A real Violet pond encounter uses the source species and level; defeat returns the healed party to Violet Center")
    game.restore("violet-day")
    from test_academy_emulator import check_academy
    check_academy(game,city,checks)
    game.restore("violet-before-gate")
    print("PASS: Violet City doors, time palettes, residents, Pokémon Center and Save/Continue",flush=True)
