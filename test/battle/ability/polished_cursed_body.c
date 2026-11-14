#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Cursed Body checks remaining PP in every slot on both sides")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_TACKLE;
    u32 slot = 0, pp = 1;
    for (u32 side = 0; side < 2; side++)
        for (u32 m = 0; m < 4; m++)
            for (u32 p = 1; p < 3; p++)
                for (u32 special = 0; special < 2; special++)
                    PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; slot = m; pp = p; move = special ? MOVE_WATER_GUN : MOVE_TACKLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == player ? 100 : 50);
            if (user == player)
                MovesWithPP({slot == 0 ? move : MOVE_SPLASH, pp}, {slot == 1 ? move : slot == 0 ? MOVE_SPLASH : MOVE_GROWL, pp},
                            {slot == 2 ? move : slot < 2 ? MOVE_GROWL : MOVE_TAIL_WHIP, pp}, {slot == 3 ? move : MOVE_TAIL_WHIP, pp});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == opponent ? 100 : 50);
            if (user == opponent)
                MovesWithPP({slot == 0 ? move : MOVE_SPLASH, pp}, {slot == 1 ? move : slot == 0 ? MOVE_SPLASH : MOVE_GROWL, pp},
                            {slot == 2 ? move : slot < 2 ? MOVE_GROWL : MOVE_TAIL_WHIP, pp}, {slot == 3 ? move : MOVE_TAIL_WHIP, pp});
        }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_CURSED_BODY, 1)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        if (!B_POLISHED_MOVE_BALANCE || pp > 1) ABILITY_POPUP(target, ABILITY_CURSED_BODY);
        else NOT ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    } THEN {
        EXPECT_EQ(user->pp[slot], pp - 1);
        EXPECT_EQ((u32)user->volatiles.disabledMove, !B_POLISHED_MOVE_BALANCE || pp > 1 ? move : MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, !B_POLISHED_MOVE_BALANCE || pp > 1 ? 3 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body rejects cleared Sleep Talk and Metronome history")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move caller = MOVE_SLEEP_TALK;
    bool32 herb = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 metronome = 0; metronome < 2; metronome++)
            for (u32 item = 0; item < 2; item++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; caller = metronome ? MOVE_METRONOME : MOVE_SLEEP_TALK; herb = item; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == player ? 100 : 50);
            if (user == player) {
                Item(herb ? ITEM_MENTAL_HERB : ITEM_NONE); Status1(caller == MOVE_SLEEP_TALK ? STATUS1_SLEEP : STATUS1_NONE);
                if (caller == MOVE_SLEEP_TALK) MovesWithPP({caller, 10}, {MOVE_TACKLE, 0});
                else MovesWithPP({caller, 10});
            }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == opponent ? 100 : 50);
            if (user == opponent) {
                Item(herb ? ITEM_MENTAL_HERB : ITEM_NONE); Status1(caller == MOVE_SLEEP_TALK ? STATUS1_SLEEP : STATUS1_NONE);
                if (caller == MOVE_SLEEP_TALK) MovesWithPP({caller, 10}, {MOVE_TACKLE, 0});
                else MovesWithPP({caller, 10});
            }
        }
    } WHEN {
        TURN { MOVE(user, caller, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? 33 : MOVE_TACKLE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, caller, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target);
        if (B_POLISHED_MOVE_BALANCE) {
            NONE_OF { ABILITY_POPUP(target, ABILITY_CURSED_BODY); ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user); }
        } else {
            ABILITY_POPUP(target, ABILITY_CURSED_BODY);
            if (herb) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user);
        }
    } THEN {
        EXPECT_EQ((u32)user->volatiles.disabledMove, !B_POLISHED_MOVE_BALANCE && !herb ? caller : MOVE_NONE);
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE && herb ? ITEM_MENTAL_HERB : ITEM_NONE);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(gLastMoves[user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT], B_POLISHED_MOVE_BALANCE ? MOVE_NONE : caller);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body retains miss protection substitute and immunity gates")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 gate = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 4; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; gate = kind; }
    GIVEN {
        PLAYER(user != player && gate == 3 ? SPECIES_GASTLY : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == player ? 100 : 50); }
        OPPONENT(user != opponent && gate == 3 ? SPECIES_GASTLY : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == opponent ? 100 : 50); }
    } WHEN {
        if (gate == 2) TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, gate == 0 ? MOVE_ROCK_THROW : MOVE_TACKLE, hit: gate != 0, WITH_RNG(RNG_CURSED_BODY, 1)); if (gate == 1) MOVE(target, MOVE_PROTECT); }
    } SCENE {
        NOT ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body still triggers through Mold Breaker attacker substitute and target knockout")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 n = 0; n < 3; n++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = n; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? (kind == 0 ? ABILITY_MOLD_BREAKER : ABILITY_NONE) : ABILITY_CURSED_BODY); Speed(user == player ? 100 : 50); HP(user != player && kind == 2 ? 1 : 200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? (kind == 0 ? ABILITY_MOLD_BREAKER : ABILITY_NONE) : ABILITY_CURSED_BODY); Speed(user == opponent ? 100 : 50); HP(user != opponent && kind == 2 ? 1 : 200); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(75); }
    } WHEN {
        if (kind == 1) TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CURSED_BODY, 1)); if (kind == 2) SEND_OUT(target, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target);
        ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_TACKLE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, 3);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body does not replace or refresh an existing Disable")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == player ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == opponent ? 50 : 100); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CURSED_BODY, 0)); }
        TURN { MOVE(target, MOVE_DISABLE, hit: TRUE); MOVE(user, MOVE_WATER_GUN, WITH_RNG(RNG_CURSED_BODY, 1)); }
        TURN { MOVE(user, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WATER_GUN, user);
        NOT ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_TACKLE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, 2);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body consumes Mental Herb only for an eligible move")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 pp = 1;
    for (u32 side = 0; side < 2; side++)
        for (u32 p = 1; p < 3; p++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; pp = p; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == player ? 100 : 50); if (user == player) { Item(ITEM_MENTAL_HERB); MovesWithPP({MOVE_TACKLE, pp}, {MOVE_SPLASH, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == opponent ? 100 : 50); if (user == opponent) { Item(ITEM_MENTAL_HERB); MovesWithPP({MOVE_TACKLE, pp}, {MOVE_SPLASH, 10}); } }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CURSED_BODY, 1)); }
        TURN { MOVE(user, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        if (!B_POLISHED_MOVE_BALANCE || pp > 1) {
            ABILITY_POPUP(target, ABILITY_CURSED_BODY);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user);
        } else {
            NONE_OF { ABILITY_POPUP(target, ABILITY_CURSED_BODY); ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user); }
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, user);
    } THEN {
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE && pp == 1 ? ITEM_MENTAL_HERB : ITEM_NONE);
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, 0);
        EXPECT_EQ(user->pp[0], pp - 1);
        EXPECT_EQ(user->pp[1], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body expires and allows the move again on either side")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == player ? 100 : 50); if (user == player) MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SPLASH, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == opponent ? 100 : 50); if (user == opponent) MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SPLASH, 10}); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CURSED_BODY, 1)); }
        TURN { MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CURSED_BODY, 0)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_CURSED_BODY);
        if (user == player) MESSAGE("Wobbuffet's move is no longer disabled!");
        else MESSAGE("The opposing Wobbuffet's move is no longer disabled!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
    } THEN {
        EXPECT_EQ(user->pp[0], 8);
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body is cleared by switching and reentry")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == player ? 100 : 50); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == opponent ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(75); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CURSED_BODY, 1)); }
        TURN { SWITCH(user, 1); }
        TURN { SWITCH(user, 0); }
        TURN { MOVE(user, MOVE_TACKLE, WITH_RNG(RNG_CURSED_BODY, 0)); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_CURSED_BODY);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body retains the source thirty percent chance for contact and noncontact moves")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_TACKLE;
    for (u32 side = 0; side < 2; side++)
        for (u32 special = 0; special < 2; special++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = special ? MOVE_WATER_GUN : MOVE_TACKLE; }
    PASSES_RANDOMLY(3, 10, RNG_CURSED_BODY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == player ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Speed(user == opponent ? 100 : 50); }
    } WHEN {
        TURN { MOVE(user, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body history also rejects Disable and Encore after Metronome hit miss and protection")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move restriction = MOVE_DISABLE;
    u32 outcome = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 encore = 0; encore < 2; encore++)
            for (u32 n = 0; n < 3; n++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; restriction = encore ? MOVE_ENCORE : MOVE_DISABLE; outcome = n; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == player ? 50 : 100); if (user == player) MovesWithPP({MOVE_METRONOME, 10}, {MOVE_SPLASH, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == opponent ? 50 : 100); if (user == opponent) MovesWithPP({MOVE_METRONOME, 10}, {MOVE_SPLASH, 10}); }
    } WHEN {
        TURN { MOVE(user, MOVE_METRONOME, hit: outcome != 1, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? (outcome == 1 ? 87 : 33) : (outcome == 1 ? MOVE_THUNDER : MOVE_TACKLE))); if (outcome == 2) MOVE(target, MOVE_PROTECT); }
        TURN { MOVE(target, restriction, hit: TRUE); MOVE(user, MOVE_SPLASH); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE || restriction == MOVE_ENCORE) { NOT ANIMATION(ANIM_TYPE_MOVE, restriction, target); MESSAGE("But it failed!"); }
        else ANIMATION(ANIM_TYPE_MOVE, restriction, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, user);
    } THEN {
        EXPECT_EQ(gLastMoves[user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT], MOVE_SPLASH);
        EXPECT_EQ((u32)user->volatiles.disabledMove, !B_POLISHED_MOVE_BALANCE && restriction == MOVE_DISABLE ? MOVE_METRONOME : MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(user->pp[1], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body does not trigger on Future Sight")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); }
    } WHEN {
        TURN { MOVE(user, MOVE_FUTURE_SIGHT, WITH_RNG(RNG_CURSED_BODY, 1)); }
        TURN {}
        TURN {}
    } SCENE {
        HP_BAR(target);
        NOT ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_NONE);
    }
}
