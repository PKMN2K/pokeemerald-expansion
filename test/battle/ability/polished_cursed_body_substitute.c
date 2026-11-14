#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Cursed Body substitute boundary uses the remaining substitute HP")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 maxHP = 200;
    for (u32 side = 0; side < 2; side++)
        for (u32 hp = 196; hp <= 204; hp += 4)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; maxHP = hp; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Level(50); Speed(100);
            if (user == player) MovesWithPP({MOVE_SEISMIC_TOSS, 2});
            else { MaxHP(maxHP); HP(maxHP); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Level(50); Speed(50);
            if (user == opponent) MovesWithPP({MOVE_SEISMIC_TOSS, 2});
            else { MaxHP(maxHP); HP(maxHP); }
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_SEISMIC_TOSS, WITH_RNG(RNG_CURSED_BODY, 1)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, user);
        if (maxHP <= 200) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
            if (B_POLISHED_MOVE_BALANCE) ABILITY_POPUP(target, ABILITY_CURSED_BODY);
            else NOT ABILITY_POPUP(target, ABILITY_CURSED_BODY);
        } else {
            NONE_OF { ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target); ABILITY_POPUP(target, ABILITY_CURSED_BODY); }
        }
    } THEN {
        EXPECT_EQ(target->hp, maxHP - maxHP / 4);
        EXPECT_EQ((u32)target->volatiles.substituteHP, maxHP > 200 ? 1 : 0);
        EXPECT_EQ((u32)target->volatiles.substitute, maxHP > 200);
        EXPECT_EQ(user->pp[0], 1);
        EXPECT_EQ((u32)user->volatiles.disabledMove, B_POLISHED_MOVE_BALANCE && maxHP <= 200 ? MOVE_SEISMIC_TOSS : MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, B_POLISHED_MOVE_BALANCE && maxHP <= 200 ? 3 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body substitute break works for contact and noncontact attacks")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_TACKLE;
    for (u32 side = 0; side < 2; side++)
        for (u32 special = 0; special < 2; special++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = special ? MOVE_WATER_GUN : MOVE_TACKLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY);
            if (user == player) MovesWithPP({move, 2});
            else { MaxHP(4); HP(4); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY);
            if (user == opponent) MovesWithPP({move, 2});
            else { MaxHP(4); HP(4); }
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, WITH_RNG(RNG_CURSED_BODY, 1)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        if (B_POLISHED_MOVE_BALANCE) ABILITY_POPUP(target, ABILITY_CURSED_BODY);
        else NOT ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    } THEN {
        EXPECT_EQ(target->hp, 3);
        EXPECT_EQ(user->pp[0], 1);
        EXPECT_EQ((u32)user->volatiles.disabledMove, B_POLISHED_MOVE_BALANCE ? move : MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, B_POLISHED_MOVE_BALANCE ? 3 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body substitute break still checks PP before consuming Mental Herb")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 pp = 1;
    for (u32 side = 0; side < 2; side++)
        for (u32 n = 1; n <= 2; n++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; pp = n; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Level(50);
            if (user == player) { Item(ITEM_MENTAL_HERB); MovesWithPP({MOVE_SEISMIC_TOSS, pp}); }
            else { MaxHP(200); HP(200); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Level(50);
            if (user == opponent) { Item(ITEM_MENTAL_HERB); MovesWithPP({MOVE_SEISMIC_TOSS, pp}); }
            else { MaxHP(200); HP(200); }
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_SEISMIC_TOSS, WITH_RNG(RNG_CURSED_BODY, 1)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        if (B_POLISHED_MOVE_BALANCE && pp > 1) {
            ABILITY_POPUP(target, ABILITY_CURSED_BODY);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user);
        } else {
            NONE_OF { ABILITY_POPUP(target, ABILITY_CURSED_BODY); ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user); }
        }
    } THEN {
        EXPECT_EQ(target->hp, 150);
        EXPECT_EQ(user->pp[0], pp - 1);
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE && pp > 1 ? ITEM_NONE : ITEM_MENTAL_HERB);
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body substitute break rejects Sleep Talk and Metronome history")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move caller = MOVE_SLEEP_TALK;
    for (u32 side = 0; side < 2; side++)
        for (u32 metronome = 0; metronome < 2; metronome++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; caller = metronome ? MOVE_METRONOME : MOVE_SLEEP_TALK; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY);
            if (user == player) {
                Item(ITEM_MENTAL_HERB); Status1(caller == MOVE_SLEEP_TALK ? STATUS1_SLEEP : STATUS1_NONE);
                if (caller == MOVE_SLEEP_TALK) MovesWithPP({caller, 10}, {MOVE_TACKLE, 0});
                else MovesWithPP({caller, 10});
            } else { MaxHP(4); HP(4); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY);
            if (user == opponent) {
                Item(ITEM_MENTAL_HERB); Status1(caller == MOVE_SLEEP_TALK ? STATUS1_SLEEP : STATUS1_NONE);
                if (caller == MOVE_SLEEP_TALK) MovesWithPP({caller, 10}, {MOVE_TACKLE, 0});
                else MovesWithPP({caller, 10});
            } else { MaxHP(4); HP(4); }
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, caller, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? 33 : MOVE_TACKLE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, caller, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        NONE_OF { ABILITY_POPUP(target, ABILITY_CURSED_BODY); ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, user); }
    } THEN {
        EXPECT_EQ(target->hp, 3);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(user->item, ITEM_MENTAL_HERB);
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body substitute bypass preserves direct HP hit activation")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 sound = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 n = 0; n < 2; n++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; sound = n; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? (sound ? ABILITY_NONE : ABILITY_INFILTRATOR) : ABILITY_CURSED_BODY);
            Level(50); SpAttack(50); SpDefense(100); MaxHP(200); HP(200);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? (sound ? ABILITY_NONE : ABILITY_INFILTRATOR) : ABILITY_CURSED_BODY);
            Level(50); SpAttack(50); SpDefense(100); MaxHP(200); HP(200);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, sound ? MOVE_HYPER_VOICE : MOVE_SEISMIC_TOSS, WITH_RNG(RNG_CURSED_BODY, 1)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, sound ? MOVE_HYPER_VOICE : MOVE_SEISMIC_TOSS, user);
        HP_BAR(target);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    } THEN {
        EXPECT_LT(target->hp, 150);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
        EXPECT_EQ((u32)target->volatiles.substitute, TRUE);
        EXPECT_EQ((u32)user->volatiles.disabledMove, sound ? MOVE_HYPER_VOICE : MOVE_SEISMIC_TOSS);
        EXPECT_EQ((u32)user->volatiles.disableTimer, 3);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body substitute damage does not leak into a following failed action")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 gate = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 n = 0; n < 3; n++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; gate = n; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY); Level(50); MaxHP(200); HP(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY); Level(50); MaxHP(200); HP(200); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_SEISMIC_TOSS, WITH_RNG(RNG_CURSED_BODY, 0)); }
        TURN {
            MOVE(user, gate == 0 ? MOVE_ROCK_THROW : gate == 1 ? MOVE_TACKLE : MOVE_SPLASH, hit: gate != 0, WITH_RNG(RNG_CURSED_BODY, 1));
            if (gate == 1) MOVE(target, MOVE_PROTECT);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        NOT ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    } THEN {
        EXPECT_EQ(target->hp, 150);
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_NONE);
        EXPECT_EQ((u32)user->volatiles.disableTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Cursed Body substitute break keeps native Z move behavior")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ABILITY_NONE : ABILITY_CURSED_BODY);
            if (user == player) Item(ITEM_WATERIUM_Z);
            else { MaxHP(4); HP(4); }
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ABILITY_NONE : ABILITY_CURSED_BODY);
            if (user == opponent) Item(ITEM_WATERIUM_Z);
            else { MaxHP(4); HP(4); }
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_WATER_GUN, gimmick: GIMMICK_Z_MOVE, WITH_RNG(RNG_CURSED_BODY, 1)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HYDRO_VORTEX, user);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        NOT ABILITY_POPUP(target, ABILITY_CURSED_BODY);
    } THEN {
        EXPECT_EQ(target->hp, 3);
        EXPECT_EQ((u32)user->volatiles.disabledMove, MOVE_NONE);
    }
}
