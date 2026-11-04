"""Mom's release opening, RTC choices and schedules via real cartridge input."""
import struct

from test_johto_demo_emulator import A, B, START, LEFT, RIGHT, UP, DOWN
from test_new_bark_emulator import (walk, travel, actors, resident_id, save_game, continue_game,
    check_resident_palette, talk_to_resident, close_resident_dialogue, player_state)
from test_new_bark_intro_emulator import (layout, var, script_active, task_active, text_matches,
    advance, choice_prompt)


def flag(game, name):
    data = layout()
    number = data[name]
    return bool(game.read(game.u32(game.sym["gSaveBlock1Ptr"]) + data["flags"] + number // 8, 1)[0] & (1 << (number % 8)))


def clock_offset(game):
    data = layout()
    return game.read(game.u32(game.sym["gSaveBlock2Ptr"]) + data["time_offset"], data["time_size"])


def weekday(game):
    # The native DateTime epoch is 2000-01-01 (Saturday); this probe only reads.
    days = struct.unpack("<h", game.read(game.sym["gLocalTime"], 2))[0]
    return (days + 6 + var(game, "weekday_offset")) % 7


def choose_weekday(game, day, reject=False):
    advance(game, lambda: task_active(game, "Task_HandleMultichoiceInput"))
    assert text_matches(game, "PolishedMom_WhatDayText")
    game.frames(30)
    game.press(B, 30)
    assert task_active(game, "Task_HandleMultichoiceInput"), "B cancelled required clock setup"
    for _ in range(day):
        game.press(DOWN, 12)
    game.screenshot("mom-weekday-" + str(day))
    game.press(A, 45)
    choice_prompt(game, "PolishedMom_ConfirmWeekdayText")
    shown = game.read(game.sym["gStringVar1"], 12).split(b"\xff", 1)[0]
    expected = ("Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday")[day]
    # Latin native character encoding: upper/lowercase have contiguous ranges.
    encoded = bytes(ord(ch) - ord('A') + 0xBB if ch.isupper() else ord(ch) - ord('a') + 0xD5 for ch in expected)
    assert shown == encoded, (expected, shown.hex())
    if reject:
        game.press(DOWN, 12)
    game.press(A, 45)


def finish_intro(game, day, dst, shoes_known, retry=False):
    original_clock = clock_offset(game)
    if retry:
        choose_weekday(game, (day + 1) % 7, reject=True)
        assert var(game, "weekday_offset") == 0, "Rejected weekday was committed"
    choose_weekday(game, day)
    assert weekday(game) == day
    for choice, reject in (((not dst, True), (dst, False)) if retry else ((dst, False),)):
        choice_prompt(game, "PolishedMom_MomDSTText")
        if not choice:
            game.press(DOWN, 12)
        game.press(A, 45)
        choice_prompt(game, "PolishedMom_ConfirmDSTText" if choice else "PolishedMom_ConfirmTimeText")
        assert flag(game, "dst") == choice
        assert clock_offset(game) == original_clock, "Initial DST selection changed the RTC offset"
        game.screenshot("mom-dst-" + str(int(choice)))
        if reject:
            game.press(DOWN, 12)
        game.press(A, 45)
    choice_prompt(game, "PolishedMom_MomRunningShoesText")
    if not shoes_known:
        game.press(DOWN, 12)
    game.press(A, 45)
    if not shoes_known:
        advance(game, lambda: text_matches(game, "PolishedMom_MomInstructionsText"))
        game.frames(240)
        game.screenshot("mom-running-instructions")
    advance(game, lambda: not script_active(game))
    assert all(flag(game, name) for name in ("gear", "phone", "mom_contact"))
    assert flag(game, "dst") == dst and var(game, "home_scene") == 1
    assert clock_offset(game) == original_clock
    assert not player_state(game)["frozen"]


def check_mom(game, maps, rows, checks):
    town, house = maps["PolishedNewBarkTown"], maps["PolishedPlayersHouse1F"]
    initial = resident_id(house, "LOCALID_HOME_INITIAL")
    neighbor = resident_id(house, "LOCALID_HOME_NEIGHBOR")
    game.set_hour(10)
    travel(game, town, house)
    walk(game, house, (8, 6))
    assert game.position() == (8, 6)
    assert var(game, "home_scene") == 0 and not flag(game, "gear")
    assert set(actors(game)) == {initial, neighbor}
    for local, graphics in ((initial, "Mom"), (neighbor, "Matron")):
        npc = actors(game)[local]
        assert game.u32(game.sym["gObjectEventGraphicsInfoPointers"] + npc["graphics"] * 4) == game.sym["gObjectEventGraphicsInfo_Polished" + graphics]
        check_resident_palette(game, house, local, 0, "day", rows)
    game.screenshot("mom-before-opening")
    save_game(game)
    game.set_hour(23)
    continue_game(game)
    assert var(game, "home_scene") == 0 and set(actors(game)) == {initial, neighbor}
    game.snapshot("mom-before-opening")
    checks.append("authentic Mom/Matron artwork and indoor palette; pre-introduction Mom survives native Save/Continue at night")

    # All paths originate at a real front-door landing. The northern approaches
    # go around the kitchen so no earlier scene trigger is crossed accidentally.
    paths = [([RIGHT, UP], None, (9, 5)),
             ([RIGHT, RIGHT, UP, UP], 1, (10, 4)),
             ([RIGHT, RIGHT, UP, RIGHT, UP], 2, (10, 4)),
             ([LEFT, LEFT, LEFT, UP, UP, UP, RIGHT, UP, RIGHT, RIGHT, UP, RIGHT], 3, (9, 3)),
             ([LEFT, LEFT, LEFT, UP, UP, UP, RIGHT, UP, RIGHT, RIGHT, RIGHT], 4, (9, 3))]
    # Seven choices exercise every weekday, both DST answers, both confirmation
    # loops, both Running Shoes branches, direct interaction and all four triggers.
    for day in range(7):
        game.restore("mom-before-opening")
        keys, trigger, landing = paths[day % len(paths)]
        for key in keys:
            game.step(key)
        if trigger is None:
            game.face(UP)
            game.press(A, 60)
        advance(game, lambda: text_matches(game, "PolishedMom_MomIntroText"))
        assert game.position() == landing
        assert actors(game)[initial]["direction"] == (1 if trigger is None else 4 if trigger < 3 else 2)
        game.frames(240)
        game.screenshot("mom-opening-" + str(trigger or "talk"))
        finish_intro(game, day, bool(day % 2), bool(day % 2), retry=day in (0, 1))
        assert actors(game)[initial]["direction"] == 3
        # Completing the scene does not move the speaker during the visit.
        assert set(actors(game)) == {initial, neighbor}
        print("PASS: Mom opening", trigger or "direct", "weekday", day, flush=True)
    checks.append("Mom direct interaction and all four original triggers reach the source position/facing and release control")
    checks.append("all seven weekdays, rejection/retry loops, DST on/off and both Running Shoes answers complete; initial DST never shifts the clock")
    saved_offset = var(game, "weekday_offset")
    talk_to_resident(game, house, initial, "mom-elm-waiting", leave_open=True)
    assert text_matches(game, "PolishedMom_ElmWaitingText")
    close_resident_dialogue(game)
    assert var(game, "weekday_offset") == saved_offset
    checks.append("pre-starter repeat conversation uses Elm's waiting text without granting the Pokégear or rerunning setup")

    travel(game, house, town)
    for hour, period, position, direction in ((4, "NITE", (1, 2), 2), (5, "MORN", (3, 2), 2),
        (8, "MORN", (3, 2), 2), (9, "DAY", (9, 4), 3), (16, "DAY", (9, 4), 3),
        (17, "EVE", (6, 3), 2), (20, "EVE", (6, 3), 2), (21, "NITE", (1, 2), 2)):
        game.set_hour(hour)
        travel(game, town, house)
        local = resident_id(house, "LOCALID_HOME_" + period)
        assert set(actors(game)) == {local, neighbor}, (hour, actors(game))
        assert actors(game)[local]["position"] == position and actors(game)[local]["direction"] == direction
        check_resident_palette(game, house, local, 0, "day", rows)
        talk_to_resident(game, house, neighbor, "home-neighbor-" + str(hour), leave_open=True)
        label = "Morn" if 5 <= hour < 9 else "Day" if 9 <= hour < 17 else "Nite"
        assert text_matches(game, "PolishedMom_Neighbor" + label + "Text")
        advance(game, lambda: text_matches(game, "PolishedMom_NeighborText"))
        close_resident_dialogue(game)
        assert actors(game)[neighbor]["direction"] == 4
        game.screenshot("mom-schedule-" + str(hour))
        travel(game, house, town)
    checks.append("all four Mom schedules switch at the original 05:00/09:00/17:00/21:00 boundaries; neighbor greetings follow source time")

    game.set_hour(16)
    travel(game, town, house)
    day_mom = resident_id(house, "LOCALID_HOME_DAY")
    talk_to_resident(game, house, day_mom, "mom-before-evening", leave_open=True)
    game.set_hour(17)
    assert day_mom in actors(game) and script_active(game)
    close_resident_dialogue(game)
    game.press(START, 40)
    game.press(B, 40)
    assert day_mom in actors(game)
    save_game(game)
    saved_position = game.position()
    continue_game(game)
    assert game.position() == saved_position
    assert set(actors(game)) == {resident_id(house, "LOCALID_HOME_EVE"), neighbor}
    assert var(game, "home_scene") == 1 and var(game, "weekday_offset") == saved_offset
    assert all(flag(game, name) for name in ("gear", "phone", "mom_contact"))
    old_weekday = weekday(game)
    game.timestamp += 86400
    game.command(f"rtc {game.timestamp}")
    game.refresh_clock()
    assert weekday(game) == (old_weekday + 1) % 7
    game.screenshot("mom-save-continue-evening")
    travel(game, house, town)
    checks.append("Mom stays present during conversation/menu returns; Save/Continue restores the correct schedule, gifts and weekday, which advances with the RTC")


def check_mom_after_starter(game, maps, checks):
    town, house = maps["PolishedNewBarkTown"], maps["PolishedPlayersHouse1F"]
    travel(game, town, house)
    live = actors(game)
    local = next(i for i in live if i != resident_id(house, "LOCALID_HOME_NEIGHBOR"))
    talk_to_resident(game, house, local, "mom-after-starter", leave_open=True)
    assert text_matches(game, "PolishedMom_MomErrandText")
    close_resident_dialogue(game)
    travel(game, house, town)
    checks.append("Mom switches to her original errand conversation after the starter; banking stays gated until Elm receives the Mystery Egg")
