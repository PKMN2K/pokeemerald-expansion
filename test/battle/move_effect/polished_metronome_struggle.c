#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Metronome and Struggle resolve source move 249 as Brick Break on either side")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_METRONOME); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_METRONOME); }
    } WHEN {
        if (enemy)
            TURN { MOVE(opponent, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? 249 : MOVE_BRICK_BREAK)); }
        else
            TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? 249 : MOVE_BRICK_BREAK)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, enemy ? opponent : player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BRICK_BREAK, enemy ? opponent : player);
        HP_BAR(enemy ? player : opponent);
    } THEN {
        EXPECT_EQ(GetMonData(GetBattlerMon(GetBattlerAtPosition(enemy ? B_POSITION_OPPONENT_LEFT : B_POSITION_PLAYER_LEFT)), MON_DATA_PP1), GetMovePP(MOVE_METRONOME) - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Metronome and Struggle call source healing aliases without native bans")
{
    u32 roll = 135;
    enum Move move = MOVE_NONE;
    PARAMETRIZE { roll = 135; move = B_POLISHED_MOVE_BALANCE ? MOVE_POLISHED_FRESH_SNACK : MOVE_SOFT_BOILED; }
    PARAMETRIZE { roll = 234; move = B_POLISHED_MOVE_BALANCE ? MOVE_POLISHED_HEALING_LIGHT : MOVE_MORNING_SUN; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(200); HP(50); Moves(MOVE_METRONOME); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? roll : move)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, player);
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(player);
    } THEN {
        EXPECT_EQ(player->hp, 150);
    }
}

SINGLE_BATTLE_TEST("Polished Metronome and Struggle call source Fury Strikes")
{
    enum Move move = B_POLISHED_MOVE_BALANCE ? MOVE_POLISHED_FURY_STRIKES : MOVE_FURY_SWIPES;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_METRONOME); Ability(ABILITY_SKILL_LINK); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? 154 : MOVE_FURY_SWIPES)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, player);
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent);
        MESSAGE("The Pokémon was hit 5 times!");
    }
}

SINGLE_BATTLE_TEST("Polished Metronome and Struggle reject known zero PP moves on both sides")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            if (!enemy) MovesWithPP({MOVE_METRONOME, 10}, {MOVE_DISARMING_VOICE, 0}, {MOVE_PLAY_ROUGH, 0}, {MOVE_MOONBLAST, 0});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            if (enemy) MovesWithPP({MOVE_METRONOME, 10}, {MOVE_DISARMING_VOICE, 0}, {MOVE_PLAY_ROUGH, 0}, {MOVE_MOONBLAST, 0});
        }
    } WHEN {
        // The runner's unrigged choice scans downward: 255 is excluded and
        // 254..252 are known. The first eligible source draw is Dark Pulse.
        if (B_POLISHED_MOVE_BALANCE)
        {
            if (enemy)
                TURN { MOVE(opponent, MOVE_METRONOME); }
            else
                TURN { MOVE(player, MOVE_METRONOME); }
        }
        else if (enemy)
            TURN { MOVE(opponent, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_DARK_PULSE)); }
        else
            TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_DARK_PULSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, enemy ? opponent : player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DARK_PULSE, enemy ? opponent : player);
        HP_BAR(enemy ? player : opponent);
    } THEN {
        EXPECT_EQ(GetMonData(GetBattlerMon(GetBattlerAtPosition(enemy ? B_POSITION_OPPONENT_LEFT : B_POSITION_PLAYER_LEFT)), MON_DATA_PP1), 9);
    }
}

SINGLE_BATTLE_TEST("Polished Metronome and Struggle exclude transformed moves instead of original party slots")
{
    GIVEN {
        PLAYER(SPECIES_DITTO) { MaxHP(1000); HP(1000); Moves(MOVE_TRANSFORM); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Moves(MOVE_METRONOME, MOVE_DISARMING_VOICE, MOVE_PLAY_ROUGH, MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, MOVE_SPLASH); }
        if (B_POLISHED_MOVE_BALANCE)
            TURN { MOVE(player, moveSlot: 0); MOVE(opponent, MOVE_SPLASH); }
        else
            TURN { MOVE(player, moveSlot: 0, WITH_RNG(RNG_METRONOME, MOVE_MOONBLAST)); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_METRONOME, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MOONBLAST, player);
        HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Polished Metronome and Struggle floor quarter maximum HP with a minimum of one")
{
    u32 hp = 0;
    bool32 enemy = FALSE;
    for (u32 n = 1; n <= 8; n++)
        PARAMETRIZE { hp = n; }
    for (u32 n = 200; n <= 204; n++)
    {
        PARAMETRIZE { hp = n; enemy = FALSE; }
        PARAMETRIZE { hp = n; enemy = TRUE; }
    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(hp); HP(hp); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(hp); HP(hp); }
    } WHEN {
        if (enemy)
            TURN { MOVE(opponent, MOVE_STRUGGLE); }
        else
            TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, enemy ? opponent : player);
        HP_BAR(enemy ? player : opponent);
        HP_BAR(enemy ? opponent : player);
    } THEN {
        u32 recoil = hp / 4;
        if (!B_POLISHED_MOVE_BALANCE && B_UPDATED_MOVE_DATA >= GEN_5 && hp % 4 >= 2)
            recoil++;
        if (recoil == 0)
            recoil = 1;
        EXPECT_EQ((enemy ? opponent : player)->hp, hp - recoil);
    }
}

SINGLE_BATTLE_TEST("Polished Metronome and Struggle recoil bypasses Rock Head and Magic Guard")
{
    enum Ability ability = ABILITY_NONE;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_ROCK_HEAD; }
    PARAMETRIZE { ability = ABILITY_MAGIC_GUARD; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(203); HP(203); Ability(ability); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        HP_BAR(opponent);
        HP_BAR(player);
    } THEN {
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 153 : 152);
    }
}

SINGLE_BATTLE_TEST("Polished Metronome and Struggle recoil is independent of dealt damage and target fainting")
{
    u32 targetHP = 1000;
    PARAMETRIZE { targetHP = 1000; }
    PARAMETRIZE { targetHP = 1; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(203); HP(203); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(targetHP); }
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        HP_BAR(opponent);
        HP_BAR(player);
    } THEN {
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 153 : 152);
    }
}

SINGLE_BATTLE_TEST("Polished Metronome and Struggle recoil applies when damaging a substitute")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(203); HP(203); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, opponent);
        HP_BAR(opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, player);
        HP_BAR(player);
    } THEN {
        EXPECT_EQ(player->hp, B_POLISHED_MOVE_BALANCE ? 153 : 152);
        EXPECT_EQ(opponent->hp, 750);
    }
}

SINGLE_BATTLE_TEST("Polished Metronome and Struggle recoil can faint the user")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(203); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_STRUGGLE); }
    } SCENE {
        HP_BAR(opponent);
        HP_BAR(player);
        MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_EQ(player->hp, 0);
    }
}
