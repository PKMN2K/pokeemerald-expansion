#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Suite 2 voluntary Pursuit intercepts exactly once including an outgoing KO")
{
    bool32 ko = FALSE;
    PARAMETRIZE { ko = FALSE; }
    PARAMETRIZE { ko = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); HP(ko ? 1 : 1000); MaxHP(1000); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_SOUNDPROOF); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_SOUNDPROOF); Moves(MOVE_PURSUIT); }
    } WHEN {
        TURN { SWITCH(player, 1); MOVE(opponent, MOVE_PURSUIT); if (ko) SEND_OUT(player, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PURSUIT, opponent);
        HP_BAR(player);
        if (ko) MESSAGE("Wobbuffet fainted!");
        SEND_IN_MESSAGE("Chansey");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_PURSUIT, opponent);
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(player->hp, 1000);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_PURSUIT) - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Pursuit before or during U-turn never attacks the replacement")
{
    bool32 first = FALSE;
    PARAMETRIZE { first = FALSE; }
    PARAMETRIZE { first = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); Speed(100); MaxHP(1000); HP(1000); Moves(MOVE_U_TURN); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_SOUNDPROOF); Speed(100); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_SOUNDPROOF); Speed(first ? 200 : 50); MaxHP(1000); HP(1000); Moves(MOVE_PURSUIT); }
    } WHEN {
        TURN { MOVE(player, MOVE_U_TURN); MOVE(opponent, MOVE_PURSUIT); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(player->hp, 1000);
        EXPECT_LT(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 1000);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_PURSUIT) - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 U-turn escapes Mean Look and clears the outgoing trap")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); MaxHP(1000); HP(1000); Moves(MOVE_SPLASH, MOVE_U_TURN); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_SOUNDPROOF); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_SOUNDPROOF); MaxHP(1000); HP(1000); Moves(MOVE_MEAN_LOOK, MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_MEAN_LOOK); }
        TURN { MOVE(player, MOVE_U_TURN); MOVE(opponent, MOVE_SPLASH); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(CanBattlerEscape(B_BATTLER_0), TRUE);
        EXPECT_EQ((u32)player->volatiles.escapePrevention, FALSE);
    }
}
