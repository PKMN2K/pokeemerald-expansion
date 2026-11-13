#include "global.h"
#include "test/battle.h"

#include "test/polished_pain_split_cases.h"

SINGLE_BATTLE_TEST("Polished Pain Split matches source rounding, caps, HP order and party updates")
{
    u32 n = 0, opponentUsesMove = 0;
    for (u32 row = 0; row < ARRAY_COUNT(sPainSplitCases); row++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { n = row; opponentUsesMove = side; }
    const struct PainSplitCase *c = &sPainSplitCases[n];
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(c->playerHP); MaxHP(c->playerMaxHP); Moves(MOVE_PAIN_SPLIT, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(c->opponentHP); MaxHP(c->opponentMaxHP); Moves(MOVE_PAIN_SPLIT, MOVE_SPLASH); }
    } WHEN {
        if (opponentUsesMove)
            TURN { MOVE(opponent, MOVE_PAIN_SPLIT); }
        else
            TURN { MOVE(player, MOVE_PAIN_SPLIT); }
    } SCENE {
        if (opponentUsesMove)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, opponent);
        else
            ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
        if (B_POLISHED_PAIN_SPLIT ? c->opponentHP > c->playerHP : opponentUsesMove)
        {
            if (c->opponentHP != c->opponentResult)
                HP_BAR(opponent, damage: c->opponentHP - c->opponentResult);
            if (c->playerHP != c->playerResult)
                HP_BAR(player, damage: c->playerHP - c->playerResult);
        }
        else
        {
            if (c->playerHP != c->playerResult)
                HP_BAR(player, damage: c->playerHP - c->playerResult);
            if (c->opponentHP != c->opponentResult)
                HP_BAR(opponent, damage: c->opponentHP - c->opponentResult);
        }
        MESSAGE("The battlers shared their pain!");
    } THEN {
        enum BattlerId user = GetBattlerAtPosition(opponentUsesMove ? B_POSITION_OPPONENT_LEFT : B_POSITION_PLAYER_LEFT);
        EXPECT_EQ(player->hp, c->playerResult);
        EXPECT_EQ(opponent->hp, c->opponentResult);
        EXPECT_EQ(GetMonData(GetBattlerMon(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), MON_DATA_HP), c->playerResult);
        EXPECT_EQ(GetMonData(GetBattlerMon(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)), MON_DATA_HP), c->opponentResult);
        EXPECT_EQ(GetMonData(GetBattlerMon(user), MON_DATA_PP1), 19);
    }
}

SINGLE_BATTLE_TEST("Polished Pain Split bypasses source Substitute without damaging the doll")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(40); MaxHP(200); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN { MOVE(player, MOVE_PAIN_SPLIT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, opponent);
        HP_BAR(opponent, damage: 50);
        if (B_POLISHED_PAIN_SPLIT)
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
            HP_BAR(opponent, damage: 55);
            HP_BAR(player, damage: -55);
            MESSAGE("The battlers shared their pain!");
        }
        else
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
    } THEN {
        EXPECT_EQ(player->hp, B_POLISHED_PAIN_SPLIT ? 95 : 40);
        EXPECT_EQ(opponent->hp, B_POLISHED_PAIN_SPLIT ? 95 : 150);
        EXPECT_EQ((u32)opponent->volatiles.substitute, TRUE);
        EXPECT_EQ((u32)opponent->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Pain Split bypasses the source Protect failure flag")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(40); MaxHP(200); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_PROTECT); MOVE(player, MOVE_PAIN_SPLIT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, opponent);
        if (B_POLISHED_PAIN_SPLIT)
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
            HP_BAR(opponent, damage: 80);
            HP_BAR(player, damage: -80);
        }
        else
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
    } THEN {
        EXPECT_EQ(player->hp, B_POLISHED_PAIN_SPLIT ? 120 : 40);
        EXPECT_EQ(opponent->hp, B_POLISHED_PAIN_SPLIT ? 120 : 200);
    }
}

SINGLE_BATTLE_TEST("Polished Pain Split cannot reach Fly or Dig even with No Guard on either side")
{
    enum Move hiddenMove = MOVE_FLY;
    u32 noGuard = 0;
    PARAMETRIZE { hiddenMove = MOVE_FLY; noGuard = 0; }
    PARAMETRIZE { hiddenMove = MOVE_FLY; noGuard = 1; }
    PARAMETRIZE { hiddenMove = MOVE_FLY; noGuard = 2; }
    PARAMETRIZE { hiddenMove = MOVE_DIG; noGuard = 0; }
    PARAMETRIZE { hiddenMove = MOVE_DIG; noGuard = 1; }
    PARAMETRIZE { hiddenMove = MOVE_DIG; noGuard = 2; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(20); MaxHP(200); Speed(50); Ability(noGuard == 1 ? ABILITY_NO_GUARD : ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(180); MaxHP(200); Speed(100); Ability(noGuard == 2 ? ABILITY_NO_GUARD : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, hiddenMove); MOVE(player, MOVE_PAIN_SPLIT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, hiddenMove, opponent);
        if (B_POLISHED_PAIN_SPLIT || !noGuard)
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player); HP_BAR(player); HP_BAR(opponent); }
        else
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
            HP_BAR(player, damage: -80);
            HP_BAR(opponent, damage: 80);
        }
    } THEN {
        EXPECT_EQ(player->hp, B_POLISHED_PAIN_SPLIT || !noGuard ? 20 : 100);
        EXPECT_EQ(opponent->hp, B_POLISHED_PAIN_SPLIT || !noGuard ? 180 : 100);
    }
}

SINGLE_BATTLE_TEST("Polished Pain Split cannot use Lock-On to bypass the source Fly and Dig guard")
{
    enum Move hiddenMove = MOVE_FLY;
    PARAMETRIZE { hiddenMove = MOVE_FLY; }
    PARAMETRIZE { hiddenMove = MOVE_DIG; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(20); MaxHP(200); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(180); MaxHP(200); Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_LOCK_ON); }
        TURN { MOVE(opponent, hiddenMove); MOVE(player, MOVE_PAIN_SPLIT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_LOCK_ON, player);
        ANIMATION(ANIM_TYPE_MOVE, hiddenMove, opponent);
        if (B_POLISHED_PAIN_SPLIT)
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player); HP_BAR(player); HP_BAR(opponent); }
        else
            ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
    } THEN {
        EXPECT_EQ(player->hp, B_POLISHED_PAIN_SPLIT ? 20 : 100);
        EXPECT_EQ(opponent->hp, B_POLISHED_PAIN_SPLIT ? 180 : 100);
    }
}

SINGLE_BATTLE_TEST("Polished Pain Split ignores Normal and Ghost damage immunities")
{
    enum Species species = SPECIES_RATTATA;
    PARAMETRIZE { species = SPECIES_RATTATA; }
    PARAMETRIZE { species = SPECIES_GASTLY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(40); MaxHP(200); }
        OPPONENT(species) { HP(200); MaxHP(200); }
    } WHEN {
        TURN { MOVE(player, MOVE_PAIN_SPLIT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
        MESSAGE("The battlers shared their pain!");
    } THEN {
        EXPECT_EQ(player->hp, 120);
        EXPECT_EQ(opponent->hp, 120);
    }
}

SINGLE_BATTLE_TEST("Polished Pain Split ignores accuracy and evasion stage changes")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(40); MaxHP(200); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); }
    } WHEN {
        for (u32 turn = 0; turn < 6; turn++)
            TURN { MOVE(opponent, MOVE_SAND_ATTACK); }
        for (u32 turn = 0; turn < 6; turn++)
            TURN { MOVE(opponent, MOVE_DOUBLE_TEAM); }
        TURN { MOVE(player, MOVE_PAIN_SPLIT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
        MESSAGE("The battlers shared their pain!");
    } THEN {
        EXPECT_EQ(player->hp, 120);
        EXPECT_EQ(opponent->hp, 120);
    }
}

SINGLE_BATTLE_TEST("Polished Pain Split still obeys Prankster immunity on Dark targets")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(40); MaxHP(200); Ability(ABILITY_PRANKSTER); }
        OPPONENT(SPECIES_UMBREON) { HP(200); MaxHP(200); }
    } WHEN {
        TURN { MOVE(player, MOVE_PAIN_SPLIT); }
    } SCENE {
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player); HP_BAR(player); HP_BAR(opponent); }
    } THEN {
        EXPECT_EQ(player->hp, 40);
        EXPECT_EQ(opponent->hp, 200);
    }
}

SINGLE_BATTLE_TEST("Polished Pain Split HP redistribution is not reflected by Counter or Mirror Coat")
{
    enum Move retaliation = MOVE_COUNTER;
    PARAMETRIZE { retaliation = MOVE_COUNTER; }
    PARAMETRIZE { retaliation = MOVE_MIRROR_COAT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(40); MaxHP(200); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); }
    } WHEN {
        TURN { MOVE(player, MOVE_PAIN_SPLIT); MOVE(opponent, retaliation); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PAIN_SPLIT, player);
        MESSAGE("The battlers shared their pain!");
        NOT ANIMATION(ANIM_TYPE_MOVE, retaliation, opponent);
    } THEN {
        EXPECT_EQ(player->hp, 120);
        EXPECT_EQ(opponent->hp, 120);
    }
}
