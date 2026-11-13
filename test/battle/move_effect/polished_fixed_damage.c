#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished fixed damage ignores level, stats, STAB and nonzero type multipliers")
{
    enum Move move = MOVE_SONIC_BOOM;
    enum Species target = SPECIES_WOBBUFFET;
    u32 level = 5, damage = 20;
    const enum Move moves[] = {MOVE_SONIC_BOOM, MOVE_DRAGON_RAGE};
    const enum Species targets[] = {SPECIES_WOBBUFFET, SPECIES_ARON, SPECIES_DRATINI};
    const u32 levels[] = {5, 50, 100};
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 s = 0; s < ARRAY_COUNT(targets); s++)
            for (u32 l = 0; l < ARRAY_COUNT(levels); l++)
                PARAMETRIZE { move = moves[m]; target = targets[s]; level = levels[l]; damage = m ? 40 : 20; }
    GIVEN {
        PLAYER(move == MOVE_SONIC_BOOM ? SPECIES_RATTATA : SPECIES_DRATINI) { Level(level); SpAttack(level == 5 ? 1 : 999); Moves(move); }
        OPPONENT(target) { HP(500); MaxHP(500); SpDefense(level == 100 ? 1 : 999); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent, damage: damage);
        NONE_OF { MESSAGE("It's super effective!"); MESSAGE("It's not very effective…"); MESSAGE("A critical hit!"); }
    } THEN {
        EXPECT_EQ(opponent->hp, 500 - damage);
        EXPECT_EQ(GetMonData(GetBattlerMon(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), MON_DATA_PP1), GetMovePP(move) - 1);
    }
}

SINGLE_BATTLE_TEST("Polished fixed damage is unchanged by Technician, Choice Specs, Multiscale and Light Screen")
{
    enum Move move = MOVE_SONIC_BOOM;
    u32 damage = 20;
    PARAMETRIZE { move = MOVE_SONIC_BOOM; damage = 20; }
    PARAMETRIZE { move = MOVE_DRAGON_RAGE; damage = 40; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_TECHNICIAN); Item(ITEM_CHOICE_SPECS); SpAttack(999); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Ability(ABILITY_MULTISCALE); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_LIGHT_SCREEN); MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent, damage: damage);
    } THEN {
        EXPECT_EQ(opponent->hp, 500 - damage);
    }
}

SINGLE_BATTLE_TEST("Polished fixed damage preserves Ghost and Fairy immunities")
{
    enum Move move = MOVE_SONIC_BOOM;
    enum Species target = SPECIES_GASTLY;
    PARAMETRIZE { move = MOVE_SONIC_BOOM; target = SPECIES_GASTLY; }
    PARAMETRIZE { move = MOVE_DRAGON_RAGE; target = SPECIES_CLEFAIRY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NO_GUARD); }
        OPPONENT(target) { HP(200); MaxHP(200); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
    } THEN {
        EXPECT_EQ(opponent->hp, 200);
    }
}

SINGLE_BATTLE_TEST("Polished fixed damage is blocked by Protect")
{
    enum Move move = MOVE_SONIC_BOOM;
    PARAMETRIZE { move = MOVE_SONIC_BOOM; }
    PARAMETRIZE { move = MOVE_DRAGON_RAGE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_PROTECT); MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, opponent);
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
    } THEN {
        EXPECT_EQ(opponent->hp, 200);
    }
}

SINGLE_BATTLE_TEST("Polished fixed damage damages Substitute instead of the target's real HP")
{
    enum Move move = MOVE_SONIC_BOOM;
    u32 damage = 20;
    PARAMETRIZE { move = MOVE_SONIC_BOOM; damage = 20; }
    PARAMETRIZE { move = MOVE_DRAGON_RAGE; damage = 40; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, opponent);
        HP_BAR(opponent, damage: 50);
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        NOT HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(opponent->hp, 150);
        EXPECT_EQ((u32)opponent->volatiles.substituteHP, 50 - damage);
    }
}

SINGLE_BATTLE_TEST("Polished fixed damage handles HP below, at and above the KO threshold")
{
    enum Move move = MOVE_SONIC_BOOM;
    u32 damage = 20, hp = 19;
    for (u32 m = 0; m < 2; m++)
        for (u32 delta = 0; delta < 3; delta++)
            PARAMETRIZE { move = m ? MOVE_DRAGON_RAGE : MOVE_SONIC_BOOM; damage = m ? 40 : 20; hp = damage - 1 + delta; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { HP(hp); MaxHP(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, move); if (hp <= damage) SEND_OUT(opponent, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent, hp: hp > damage ? 1 : 0);
    }
}

AI_SINGLE_BATTLE_TEST("Polished fixed damage AI uses the payload and avoids immune targets")
{
    bool32 fairy = FALSE;
    PARAMETRIZE { fairy = FALSE; }
    PARAMETRIZE { fairy = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | AI_FLAG_CHECK_VIABILITY);
        PLAYER(fairy ? SPECIES_CLEFAIRY : SPECIES_WOBBUFFET) { HP(30); MaxHP(200); Defense(999); SpDefense(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(5); Attack(1); SpAttack(1); Moves(MOVE_SONIC_BOOM, MOVE_DRAGON_RAGE, MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, fairy ? MOVE_SONIC_BOOM : MOVE_DRAGON_RAGE); }
    }
}

SINGLE_BATTLE_TEST("Polished fixed damage can reach Fly and Dig with No Guard")
{
    enum Move move = MOVE_SONIC_BOOM, hiddenMove = MOVE_FLY;
    u32 damage = 20, noGuard = 0;
    for (u32 m = 0; m < 2; m++)
        for (u32 h = 0; h < 2; h++)
            for (u32 g = 0; g < 2; g++)
                PARAMETRIZE { move = m ? MOVE_DRAGON_RAGE : MOVE_SONIC_BOOM; damage = m ? 40 : 20; hiddenMove = h ? MOVE_DIG : MOVE_FLY; noGuard = g; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); Ability(noGuard ? ABILITY_NO_GUARD : ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); Speed(100); }
    } WHEN {
        TURN { MOVE(opponent, hiddenMove); MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, hiddenMove, opponent);
        if (noGuard)
        {
            ANIMATION(ANIM_TYPE_MOVE, move, player);
            HP_BAR(opponent, damage: damage);
        }
        else
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, player); HP_BAR(opponent); }
    } THEN {
        EXPECT_EQ(opponent->hp, noGuard ? 200 - damage : 200);
    }
}
