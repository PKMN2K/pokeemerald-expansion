#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Belly Drum stage limits follow the effective direction on both sides")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    s32 initial = 0;
    bool32 succeeds;
    for (u32 side = 0; side < 2; side++)
        for (u32 contrary = 0; contrary < 2; contrary++)
            for (s32 stage = -6; stage <= 6; stage += 2)
                if (stage == -6 || stage == -4 || stage == 0 || stage == 4 || stage == 6)
                    PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = contrary ? ABILITY_CONTRARY : ABILITY_NONE; initial = stage; }
    succeeds = initial != (B_POLISHED_MOVE_BALANCE && ability == ABILITY_CONTRARY ? -6 : 6);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            MaxHP(user == player ? 200 : 1000); HP(user == player ? 200 : 1000);
            if (user == player) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            MaxHP(user == opponent ? 200 : 1000); HP(user == opponent ? 200 : 1000);
            if (user == opponent) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
    } WHEN {
        for (u32 i = 0; i < (initial < 0 ? -initial : initial) / 2; i++)
        {
            if ((initial > 0) == (ability != ABILITY_CONTRARY))
                TURN { MOVE(user, MOVE_SWORDS_DANCE); }
            else
                TURN { MOVE(target, MOVE_CHARM, hit: TRUE); }
        }
        TURN { MOVE(user, MOVE_BELLY_DRUM); }
    } SCENE {
        if (succeeds)
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user);
            HP_BAR(user, damage: 100);
        }
        else
        {
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user); }
            MESSAGE("But it failed!");
        }
    } THEN {
        EXPECT_EQ(user->hp, succeeds ? 100 : 200);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(user->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (succeeds ? (ability == ABILITY_CONTRARY ? -6 : 6) : initial));
    }
}

SINGLE_BATTLE_TEST("Polished Belly Drum requires more than floored half maximum HP and preserves failed PP costs")
{
    struct BattlePokemon *user = player;
    enum Ability ability = ABILITY_NONE;
    u32 maximum = 100, current = 49;
    bool32 succeeds;
    for (u32 side = 0; side < 2; side++)
        for (u32 contrary = 0; contrary < 2; contrary++)
            for (u32 maximumHP = 100; maximumHP <= 101; maximumHP++)
                for (u32 sample = 0; sample < 4; sample++)
                    PARAMETRIZE { user = side ? opponent : player; ability = contrary ? ABILITY_CONTRARY : ABILITY_NONE; maximum = maximumHP; current = sample == 3 ? maximumHP : 49 + sample; }
    succeeds = current > maximum / 2;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            MaxHP(user == player ? maximum : 1000); HP(user == player ? current : 1000);
            if (user == player) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            MaxHP(user == opponent ? maximum : 1000); HP(user == opponent ? current : 1000);
            if (user == opponent) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
    } WHEN {
        TURN { MOVE(user, MOVE_BELLY_DRUM); }
    } SCENE {
        if (succeeds) { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user, damage: maximum / 2); }
        else { MESSAGE("But it failed!"); NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user); } }
    } THEN {
        EXPECT_EQ(user->hp, succeeds ? current - maximum / 2 : current);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(user->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (succeeds ? (ability == ABILITY_CONTRARY ? -6 : 6) : 0));
    }
}

SINGLE_BATTLE_TEST("Polished Belly Drum has a minimum one HP cost and never self-faints")
{
    struct BattlePokemon *user = player;
    enum Ability ability = ABILITY_NONE;
    u32 maximum = 1;
    for (u32 side = 0; side < 2; side++)
        for (u32 contrary = 0; contrary < 2; contrary++)
            for (u32 hp = 1; hp <= 3; hp++)
                PARAMETRIZE { user = side ? opponent : player; ability = contrary ? ABILITY_CONTRARY : ABILITY_NONE; maximum = hp; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            MaxHP(user == player ? maximum : 1000); HP(user == player ? maximum : 1000);
            if (user == player) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            MaxHP(user == opponent ? maximum : 1000); HP(user == opponent ? maximum : 1000);
            if (user == opponent) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
    } WHEN {
        TURN { MOVE(user, MOVE_BELLY_DRUM); }
    } SCENE {
        if (maximum > 1) { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user, damage: 1); }
        else { MESSAGE("But it failed!"); NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user); } }
    } THEN {
        EXPECT_EQ(user->hp, maximum == 1 ? 1 : maximum - 1);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(user->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (maximum > 1 ? (ability == ABILITY_CONTRARY ? -6 : 6) : 0));
    }
}

SINGLE_BATTLE_TEST("Polished Belly Drum preserves other stats while reaching the Attack limit")
{
    struct BattlePokemon *user = player;
    enum Ability ability = ABILITY_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 contrary = 0; contrary < 2; contrary++)
            PARAMETRIZE { user = side ? opponent : player; ability = contrary ? ABILITY_CONTRARY : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            MaxHP(user == player ? 200 : 1000); HP(user == player ? 200 : 1000);
            if (user == player) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_ACID_ARMOR, 10}, {MOVE_AGILITY, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            MaxHP(user == opponent ? 200 : 1000); HP(user == opponent ? 200 : 1000);
            if (user == opponent) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_ACID_ARMOR, 10}, {MOVE_AGILITY, 10});
        }
    } WHEN {
        TURN { MOVE(user, MOVE_ACID_ARMOR); }
        TURN { MOVE(user, MOVE_AGILITY); }
        TURN { MOVE(user, MOVE_BELLY_DRUM); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user);
        HP_BAR(user, damage: 100);
    } THEN {
        EXPECT_EQ(user->statStages[STAT_ATK], ability == ABILITY_CONTRARY ? MIN_STAT_STAGE : MAX_STAT_STAGE);
        EXPECT_EQ(user->statStages[STAT_DEF], DEFAULT_STAT_STAGE + (ability == ABILITY_CONTRARY ? -2 : 2));
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (ability == ABILITY_CONTRARY ? -2 : 2));
        EXPECT_EQ(user->statStages[STAT_SPATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->statStages[STAT_SPDEF], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->statStages[STAT_EVASION], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Belly Drum rejects a repeated capped attempt after HP is restored")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 contrary = 0; contrary < 2; contrary++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = contrary ? ABILITY_CONTRARY : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            MaxHP(user == player ? 200 : 1000); HP(user == player ? 200 : 1000);
            if (user == player) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            MaxHP(user == opponent ? 200 : 1000); HP(user == opponent ? 200 : 1000);
            if (user == opponent) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
    } WHEN {
        TURN { MOVE(user, MOVE_BELLY_DRUM); }
        TURN { MOVE(target, MOVE_HEAL_PULSE); }
        TURN { MOVE(user, MOVE_BELLY_DRUM); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user);
        HP_BAR(user, damage: 100);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HEAL_PULSE, target);
        HP_BAR(user, hp: 200);
        if (!B_POLISHED_MOVE_BALANCE && ability == ABILITY_CONTRARY)
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user);
            HP_BAR(user, damage: 100);
        }
        else
        {
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user); }
            MESSAGE("But it failed!");
        }
    } THEN {
        EXPECT_EQ(user->hp, !B_POLISHED_MOVE_BALANCE && ability == ABILITY_CONTRARY ? 100 : 200);
        EXPECT_EQ(user->pp[0], 8);
        EXPECT_EQ(user->statStages[STAT_ATK], ability == ABILITY_CONTRARY ? MIN_STAT_STAGE : MAX_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished Belly Drum remains usable after switching resets the Attack stages")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 contrary = 0; contrary < 2; contrary++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = contrary ? ABILITY_CONTRARY : ABILITY_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            MaxHP(user == player ? 200 : 1000); HP(user == player ? 200 : 1000);
            if (user == player) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            MaxHP(user == opponent ? 200 : 1000); HP(user == opponent ? 200 : 1000);
            if (user == opponent) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); }
    } WHEN {
        TURN { MOVE(user, MOVE_BELLY_DRUM); }
        TURN { MOVE(target, MOVE_HEAL_PULSE); }
        TURN { SWITCH(user, 1); }
        TURN { SWITCH(user, 0); }
        TURN { MOVE(user, MOVE_BELLY_DRUM); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user);
        HP_BAR(user, damage: 100);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HEAL_PULSE, target);
        HP_BAR(user, hp: 200);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user);
        HP_BAR(user, damage: 100);
    } THEN {
        EXPECT_EQ(user->hp, 100);
        EXPECT_EQ(user->pp[0], 8);
        EXPECT_EQ(user->statStages[STAT_ATK], ability == ABILITY_CONTRARY ? MIN_STAT_STAGE : MAX_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished Belly Drum uses the normal direction when Neutralizing Gas suppresses Contrary")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_CONTRARY;
    s32 initial = -6;
    for (u32 side = 0; side < 2; side++)
        for (s32 stage = -6; stage <= 6; stage += 12)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; initial = stage; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NEUTRALIZING_GAS); Speed(user == player ? 100 : 50);
            MaxHP(user == player ? 200 : 1000); HP(user == player ? 200 : 1000);
            if (user == player) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NEUTRALIZING_GAS); Speed(user == opponent ? 100 : 50);
            MaxHP(user == opponent ? 200 : 1000); HP(user == opponent ? 200 : 1000);
            if (user == opponent) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
        }
    } WHEN {
        for (u32 i = 0; i < 3; i++)
        {
            if (initial > 0) TURN { MOVE(user, MOVE_SWORDS_DANCE); }
            else TURN { MOVE(target, MOVE_CHARM, hit: TRUE); }
        }
        TURN { MOVE(user, MOVE_BELLY_DRUM); }
    } SCENE {
        if (initial < 0) { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user, damage: 100); }
        else { NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user); } MESSAGE("But it failed!"); }
    } THEN {
        EXPECT_EQ(user->hp, initial < 0 ? 100 : 200);
        EXPECT_EQ(user->statStages[STAT_ATK], MAX_STAT_STAGE);
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Belly Drum retains native Contrary limits and healing for Z-moves")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_CONTRARY;
    s32 initial = -6;
    for (u32 side = 0; side < 2; side++)
        for (s32 stage = -6; stage <= 6; stage += 12)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; initial = stage; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            MaxHP(user == player ? 200 : 1000); HP(user == player ? 60 : 1000);
            if (user == player) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
            Item(user == player ? ITEM_NORMALIUM_Z : ITEM_NONE);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            MaxHP(user == opponent ? 200 : 1000); HP(user == opponent ? 60 : 1000);
            if (user == opponent) MovesWithPP({MOVE_BELLY_DRUM, 10}, {MOVE_SWORDS_DANCE, 10});
            Item(user == opponent ? ITEM_NORMALIUM_Z : ITEM_NONE);
        }
    } WHEN {
        for (u32 i = 0; i < (initial < 0 ? -initial : initial) / 2; i++)
        {
            if ((initial > 0) == (ability != ABILITY_CONTRARY))
                TURN { MOVE(user, MOVE_SWORDS_DANCE); }
            else
                TURN { MOVE(target, MOVE_CHARM, hit: TRUE); }
        }
        TURN { MOVE(user, MOVE_BELLY_DRUM, gimmick: GIMMICK_Z_MOVE); }
    } SCENE {
        HP_BAR(user, hp: 200);
        if (initial < 0) { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user, damage: 100); }
        else { NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_BELLY_DRUM, user); HP_BAR(user); } MESSAGE("But it failed!"); }
    } THEN {
        EXPECT_EQ(user->hp, initial < 0 ? 100 : 200);
        EXPECT_EQ(user->statStages[STAT_ATK], DEFAULT_STAT_STAGE + initial);
        EXPECT_EQ(user->pp[0], 9);
    }
}
