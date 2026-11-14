#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished move restrictions Disable expires after the source action and end-turn ticks")
{
    struct BattlePokemon *target = player, *user = opponent;
    bool32 targetFirst = FALSE;
    u32 futureTurns;
    for (u32 side = 0; side < 2; side++)
        for (u32 first = 0; first < 2; first++)
            PARAMETRIZE { target = side ? opponent : player; user = side ? player : opponent; targetFirst = first; }
    futureTurns = B_POLISHED_MOVE_BALANCE && targetFirst ? 4 : 3;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed((target == player) == targetFirst ? 100 : 50); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed((target == opponent) == targetFirst ? 100 : 50); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_DISABLE, hit: TRUE); MOVE(target, MOVE_SPLASH); }
        for (u32 i = 0; i < futureTurns; i++)
            TURN { MOVE(target, MOVE_TACKLE); }
        TURN { MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
        if (targetFirst)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, user);
        for (u32 i = 0; i < futureTurns; i++)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
        if (target == player)
            MESSAGE("Wobbuffet's move is no longer disabled!");
        else
            MESSAGE("The opposing Wobbuffet's move is no longer disabled!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.disableTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished move restrictions Encore always starts at four ticks and overrides a slower target")
{
    struct BattlePokemon *target = player, *user = opponent;
    bool32 targetFirst = FALSE;
    u32 futureTurns;
    for (u32 side = 0; side < 2; side++)
        for (u32 first = 0; first < 2; first++)
            PARAMETRIZE { target = side ? opponent : player; user = side ? player : opponent; targetFirst = first; }
    futureTurns = B_POLISHED_MOVE_BALANCE || targetFirst ? 3 : 2;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed((target == player) == targetFirst ? 100 : 50); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed((target == opponent) == targetFirst ? 100 : 50); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_ENCORE); MOVE(target, targetFirst ? MOVE_SPLASH : MOVE_TACKLE); }
        for (u32 i = 0; i < futureTurns; i++)
            TURN { MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(target, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
        if (targetFirst)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, user);
        if (!targetFirst)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
        for (u32 i = 0; i < futureTurns; i++)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
        if (target == player)
            MESSAGE("Wobbuffet ended its encore!");
        else
            MESSAGE("The opposing Wobbuffet ended its encore!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.encoreTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished move restrictions reject an active reapplication without refreshing its counter")
{
    enum Move restriction = MOVE_DISABLE;
    bool32 targetFirst = FALSE;
    for (u32 kind = 0; kind < 2; kind++)
        for (u32 first = 0; first < 2; first++)
            PARAMETRIZE { restriction = kind ? MOVE_ENCORE : MOVE_DISABLE; targetFirst = first; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(targetFirst ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(targetFirst ? 50 : 100); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, restriction, hit: TRUE); }
        TURN { MOVE(player, restriction == MOVE_ENCORE ? MOVE_SPLASH : MOVE_TACKLE); MOVE(opponent, restriction, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, restriction, opponent);
        NOT ANIMATION(ANIM_TYPE_MOVE, restriction, opponent);
        MESSAGE("But it failed!");
    } THEN {
        if (restriction == MOVE_DISABLE) {
            EXPECT_EQ((u32)player->volatiles.disabledMove, MOVE_SPLASH);
            EXPECT_EQ((u32)player->volatiles.disableTimer, B_POLISHED_MOVE_BALANCE && targetFirst ? 3 : 2);
        } else {
            EXPECT_EQ((u32)player->volatiles.encoredMove, MOVE_SPLASH);
            EXPECT_EQ((u32)player->volatiles.encoreTimer, B_POLISHED_MOVE_BALANCE || targetFirst ? 2 : 1);
        }
    }
}

SINGLE_BATTLE_TEST("Polished move restrictions reject missing history and exhausted last-move PP")
{
    enum Move restriction = MOVE_DISABLE;
    bool32 exhausted = FALSE;
    for (u32 kind = 0; kind < 2; kind++)
        for (u32 pp = 0; pp < 2; pp++)
            PARAMETRIZE { restriction = kind ? MOVE_ENCORE : MOVE_DISABLE; exhausted = pp; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MovesWithPP({MOVE_SPLASH, 1}, {MOVE_TACKLE, 10}, {MOVE_GROWL, 10}, {MOVE_TAIL_WHIP, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
    } WHEN {
        if (exhausted)
            TURN { MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(opponent, restriction, hit: TRUE); MOVE(player, MOVE_TACKLE); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, restriction, opponent);
        MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ((u32)player->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)player->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ((u32)player->volatiles.disableTimer, 0);
        EXPECT_EQ((u32)player->volatiles.encoreTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished move restrictions Encore ends early when its move exhausts PP")
{
    struct BattlePokemon *target = player, *user = opponent;
    PARAMETRIZE { target = player; user = opponent; }
    PARAMETRIZE { target = opponent; user = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == player ? 50 : 100); MovesWithPP({MOVE_SPLASH, 2}, {MOVE_TACKLE, 10}, {MOVE_ENCORE, 5}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == opponent ? 50 : 100); MovesWithPP({MOVE_SPLASH, 2}, {MOVE_TACKLE, 10}, {MOVE_ENCORE, 5}); }
    } WHEN {
        TURN { MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_ENCORE); MOVE(target, MOVE_TACKLE); }
        TURN { MOVE(target, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
        if (target == player)
            MESSAGE("Wobbuffet ended its encore!");
        else
            MESSAGE("The opposing Wobbuffet ended its encore!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
    } THEN {
        EXPECT_EQ(target->pp[0], 0);
        EXPECT_EQ(target->pp[1], 9);
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.encoreTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished move restrictions Mental Herb clears both the move and timer on either side")
{
    struct BattlePokemon *target = player, *user = opponent;
    enum Move restriction = MOVE_DISABLE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { target = side ? opponent : player; user = side ? player : opponent; restriction = kind ? MOVE_ENCORE : MOVE_DISABLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == player ? 50 : 100); Item(target == player ? ITEM_MENTAL_HERB : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == opponent ? 50 : 100); Item(target == opponent ? ITEM_MENTAL_HERB : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, restriction, hit: TRUE); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, restriction, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
    } THEN {
        EXPECT_EQ(target->item, ITEM_NONE);
        EXPECT_EQ((u32)target->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.disableTimer, 0);
        EXPECT_EQ((u32)target->volatiles.encoreTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished move restrictions switching clears the restriction and does not restore it on return")
{
    struct BattlePokemon *target = player, *user = opponent;
    enum Move restriction = MOVE_DISABLE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { target = side ? opponent : player; user = side ? player : opponent; restriction = kind ? MOVE_ENCORE : MOVE_DISABLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == player ? 100 : 50); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == player ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == opponent ? 100 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == opponent ? 100 : 50); }
    } WHEN {
        TURN { MOVE(target, MOVE_SPLASH); MOVE(user, restriction, hit: TRUE); }
        TURN { SWITCH(target, 1); }
        TURN { SWITCH(target, 0); }
        TURN { MOVE(target, restriction == MOVE_DISABLE ? MOVE_SPLASH : MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, restriction, user);
        ANIMATION(ANIM_TYPE_MOVE, restriction == MOVE_DISABLE ? MOVE_SPLASH : MOVE_TACKLE, target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.disableTimer, 0);
        EXPECT_EQ((u32)target->volatiles.encoreTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished move restrictions tick even when paralysis interrupts the affected action")
{
    struct BattlePokemon *target = player, *user = opponent;
    enum Move restriction = MOVE_DISABLE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { target = side ? opponent : player; user = side ? player : opponent; restriction = kind ? MOVE_ENCORE : MOVE_DISABLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == player ? 50 : 100); Status1(target == player ? STATUS1_PARALYSIS : STATUS1_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == opponent ? 50 : 100); Status1(target == opponent ? STATUS1_PARALYSIS : STATUS1_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SPLASH, WITH_RNG(RNG_PARALYSIS, FALSE)); }
        TURN { MOVE(user, restriction, hit: TRUE); MOVE(target, MOVE_TACKLE, WITH_RNG(RNG_PARALYSIS, TRUE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, restriction, user);
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
        }
        if (target == player)
            MESSAGE("Wobbuffet couldn't move because it's paralyzed!");
        else
            MESSAGE("The opposing Wobbuffet couldn't move because it's paralyzed!");
    } THEN {
        if (restriction == MOVE_DISABLE) {
            EXPECT_EQ((u32)target->volatiles.disabledMove, MOVE_SPLASH);
            EXPECT_EQ((u32)target->volatiles.disableTimer, 3);
        } else {
            EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_SPLASH);
            EXPECT_EQ((u32)target->volatiles.encoreTimer, B_POLISHED_MOVE_BALANCE ? 3 : 2);
        }
    }
}
