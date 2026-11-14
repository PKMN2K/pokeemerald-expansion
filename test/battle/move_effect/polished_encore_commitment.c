#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Encore commitment survives Mental Herb in every move slot on both sides")
{
    struct BattlePokemon *target = player, *user = opponent;
    enum Move remembered = MOVE_TACKLE, chosen = MOVE_SPLASH;
    enum Ability ability = ABILITY_NONE;
    u32 slot = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 m = 0; m < 4; m++)
            for (u32 attack = 0; attack < 2; attack++)
                for (u32 unnerve = 0; unnerve < 2; unnerve++)
                    PARAMETRIZE {
                        target = side ? opponent : player; user = side ? player : opponent;
                        slot = m; remembered = attack ? MOVE_TACKLE : MOVE_SPLASH; chosen = attack ? MOVE_SPLASH : MOVE_TACKLE;
                        ability = unnerve ? ABILITY_UNNERVE : ABILITY_NONE;
                    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(target == player ? ABILITY_NONE : ability); Speed(target == player ? 50 : 100); MaxHP(1000); HP(1000);
            Item(target == player ? ITEM_MENTAL_HERB : ITEM_NONE);
            if (target == player)
                MovesWithPP({slot == 0 ? remembered : chosen, 10}, {slot == 1 ? remembered : slot == 0 ? chosen : MOVE_GROWL, 10},
                            {slot == 2 ? remembered : slot < 2 ? MOVE_GROWL : MOVE_TAIL_WHIP, 10}, {slot == 3 ? remembered : MOVE_TAIL_WHIP, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(target == opponent ? ABILITY_NONE : ability); Speed(target == opponent ? 50 : 100); MaxHP(1000); HP(1000);
            Item(target == opponent ? ITEM_MENTAL_HERB : ITEM_NONE);
            if (target == opponent)
                MovesWithPP({slot == 0 ? remembered : chosen, 10}, {slot == 1 ? remembered : slot == 0 ? chosen : MOVE_GROWL, 10},
                            {slot == 2 ? remembered : slot < 2 ? MOVE_GROWL : MOVE_TAIL_WHIP, 10}, {slot == 3 ? remembered : MOVE_TAIL_WHIP, 10});
        }
    } WHEN {
        TURN { MOVE(target, remembered); }
        TURN { MOVE(user, MOVE_ENCORE); MOVE(target, chosen); }
        TURN { MOVE(target, chosen); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, remembered, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, target);
        ANIMATION(ANIM_TYPE_MOVE, B_POLISHED_MOVE_BALANCE ? remembered : chosen, target);
        ANIMATION(ANIM_TYPE_MOVE, chosen, target);
    } THEN {
        EXPECT_EQ(target->pp[slot], B_POLISHED_MOVE_BALANCE ? 8 : 9);
        EXPECT_EQ(target->pp[slot == 0 ? 1 : 0], B_POLISHED_MOVE_BALANCE ? 9 : 8);
        EXPECT_EQ(target->item, ITEM_NONE);
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.encoreTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Encore commitment does not carry into the next turn when the target already acted")
{
    struct BattlePokemon *target = player, *user = opponent;
    enum Ability ability = ABILITY_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 unnerve = 0; unnerve < 2; unnerve++)
            PARAMETRIZE { target = side ? opponent : player; user = side ? player : opponent; ability = unnerve ? ABILITY_UNNERVE : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(target == player ? ABILITY_NONE : ability); Speed(target == player ? 100 : 50);
            Item(target == player ? ITEM_MENTAL_HERB : ITEM_NONE);
            if (target == player) MovesWithPP({MOVE_TACKLE, 3}, {MOVE_SPLASH, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(target == opponent ? ABILITY_NONE : ability); Speed(target == opponent ? 100 : 50);
            Item(target == opponent ? ITEM_MENTAL_HERB : ITEM_NONE);
            if (target == opponent) MovesWithPP({MOVE_TACKLE, 3}, {MOVE_SPLASH, 10});
        }
    } WHEN {
        TURN { MOVE(target, MOVE_TACKLE); MOVE(user, MOVE_ENCORE); }
        TURN { MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
    } THEN {
        EXPECT_EQ(target->pp[0], 2);
        EXPECT_EQ(target->pp[1], 9);
        EXPECT_EQ(target->item, ITEM_NONE);
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Encore commitment preserves the active restriction without a curing item")
{
    struct BattlePokemon *target = player, *user = opponent;
    PARAMETRIZE { target = player; user = opponent; }
    PARAMETRIZE { target = opponent; user = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == player ? 50 : 100); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == opponent ? 50 : 100); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(target, MOVE_TACKLE); }
        TURN { MOVE(user, MOVE_ENCORE); MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(target, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_TACKLE);
        EXPECT_EQ((u32)target->volatiles.encoreTimer, B_POLISHED_MOVE_BALANCE ? 2 : 1);
    }
}

SINGLE_BATTLE_TEST("Polished Encore commitment leaves the selected move and herb intact when Encore fails")
{
    struct BattlePokemon *target = player, *user = opponent;
    u32 reason = 0;
    enum Move previous;
    for (u32 side = 0; side < 2; side++)
        for (u32 n = 0; n < 3; n++)
            PARAMETRIZE { target = side ? opponent : player; user = side ? player : opponent; reason = n; }
    previous = reason == 2 ? MOVE_SLEEP_TALK : MOVE_SPLASH;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(target == player ? 50 : 100); Item(target == player ? ITEM_MENTAL_HERB : ITEM_NONE);
            if (target == player) MovesWithPP({previous, reason == 2 ? 2 : 1}, {MOVE_TACKLE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(target == opponent ? 50 : 100); Item(target == opponent ? ITEM_MENTAL_HERB : ITEM_NONE);
            if (target == opponent) MovesWithPP({previous, reason == 2 ? 2 : 1}, {MOVE_TACKLE, 10});
        }
    } WHEN {
        if (reason != 0)
            TURN { MOVE(target, previous); }
        TURN { MOVE(user, MOVE_ENCORE); MOVE(target, MOVE_TACKLE); }
    } SCENE {
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, user);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, target);
        }
        MESSAGE("But it failed!");
        if (reason == 2) MESSAGE("But it failed!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
    } THEN {
        EXPECT_EQ(target->pp[1], 9);
        EXPECT_EQ(target->item, ITEM_MENTAL_HERB);
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Encore commitment spends no PP on a paralyzed action and allows the next choice")
{
    struct BattlePokemon *target = player, *user = opponent;
    PARAMETRIZE { target = player; user = opponent; }
    PARAMETRIZE { target = opponent; user = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(target == player ? 50 : 100); Status1(target == player ? STATUS1_PARALYSIS : STATUS1_NONE);
            Item(target == player ? ITEM_MENTAL_HERB : ITEM_NONE);
            if (target == player) MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SPLASH, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(target == opponent ? 50 : 100); Status1(target == opponent ? STATUS1_PARALYSIS : STATUS1_NONE);
            Item(target == opponent ? ITEM_MENTAL_HERB : ITEM_NONE);
            if (target == opponent) MovesWithPP({MOVE_TACKLE, 10}, {MOVE_SPLASH, 10});
        }
    } WHEN {
        TURN { MOVE(target, MOVE_TACKLE, WITH_RNG(RNG_PARALYSIS, FALSE)); }
        TURN { MOVE(user, MOVE_ENCORE); MOVE(target, MOVE_SPLASH, WITH_RNG(RNG_PARALYSIS, TRUE)); }
        TURN { MOVE(target, MOVE_SPLASH, WITH_RNG(RNG_PARALYSIS, FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, target);
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
        }
        if (target == player) MESSAGE("Wobbuffet couldn't move because it's paralyzed!");
        else MESSAGE("The opposing Wobbuffet couldn't move because it's paralyzed!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
    } THEN {
        EXPECT_EQ(target->pp[0], 9);
        EXPECT_EQ(target->pp[1], 9);
        EXPECT_EQ(target->item, ITEM_NONE);
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Encore commitment keeps Disable and Attract herb cures usable under Unnerve")
{
    struct BattlePokemon *target = player, *user = opponent;
    enum Move restriction = MOVE_DISABLE;
    enum Ability ability = ABILITY_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            for (u32 unnerve = 0; unnerve < 2; unnerve++)
                PARAMETRIZE { target = side ? opponent : player; user = side ? player : opponent; restriction = kind ? MOVE_ATTRACT : MOVE_DISABLE; ability = unnerve ? ABILITY_UNNERVE : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(target == player ? ABILITY_NONE : ability); Speed(target == player ? 50 : 100); Gender(target == player ? MON_FEMALE : MON_MALE);
            Item(target == player ? ITEM_MENTAL_HERB : ITEM_NONE);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(target == opponent ? ABILITY_NONE : ability); Speed(target == opponent ? 50 : 100); Gender(target == opponent ? MON_FEMALE : MON_MALE);
            Item(target == opponent ? ITEM_MENTAL_HERB : ITEM_NONE);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, restriction, hit: TRUE); MOVE(target, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, restriction, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, target);
    } THEN {
        EXPECT_EQ(target->item, ITEM_NONE);
        EXPECT_EQ((u32)target->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.disableTimer, 0);
        EXPECT_EQ((u32)target->volatiles.infatuation, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Encore commitment does not persist through switching and reentry after a cure")
{
    struct BattlePokemon *target = player, *user = opponent;
    PARAMETRIZE { target = player; user = opponent; }
    PARAMETRIZE { target = opponent; user = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == player ? 50 : 100); Item(target == player ? ITEM_MENTAL_HERB : ITEM_NONE); MaxHP(1000); HP(1000); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(target == opponent ? 50 : 100); Item(target == opponent ? ITEM_MENTAL_HERB : ITEM_NONE); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); }
    } WHEN {
        TURN { MOVE(target, MOVE_TACKLE); }
        TURN { MOVE(user, MOVE_ENCORE); MOVE(target, MOVE_SPLASH); }
        TURN { SWITCH(target, 1); }
        TURN { SWITCH(target, 0); }
        TURN { MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, target);
        ANIMATION(ANIM_TYPE_MOVE, B_POLISHED_MOVE_BALANCE ? MOVE_TACKLE : MOVE_SPLASH, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, target);
    } THEN {
        EXPECT_EQ(target->item, ITEM_NONE);
        EXPECT_EQ((u32)target->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ((u32)target->volatiles.encoreTimer, 0);
    }
}
