#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished move PP starts and depletes at the selected profile's values")
{
    bool32 source;
    PARAMETRIZE { source = TRUE; }
    PARAMETRIZE { source = FALSE; }
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, source);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_HONE_CLAWS, MOVE_ROCK_BLAST); Attack(10); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_HONE_CLAWS); HP(1000); MaxHP(1000); Defense(255); }
    } WHEN {
        TURN { MOVE(player, MOVE_HONE_CLAWS); MOVE(opponent, MOVE_HONE_CLAWS); }
        TURN { MOVE(player, MOVE_ROCK_BLAST); MOVE(opponent, MOVE_HONE_CLAWS); }
    } THEN {
        EXPECT_EQ(player->pp[0], source ? 39 : 14);
        EXPECT_EQ(player->pp[1], source ? 24 : 9);
        EXPECT_EQ(opponent->pp[0], source ? 38 : 13);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_PP2), source ? 24 : 9);
    }
}

SINGLE_BATTLE_TEST("Polished move PP Max Elixir uses source maxima on both battle sides")
{
    u32 bonuses = 0xff;
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { MovesWithPP({MOVE_GROWL, 0}, {MOVE_ROCK_BLAST, 0}, {MOVE_RAPID_SPIN, 0}, {MOVE_RECOVER, 0}); }
        SetMonData(&PLAYER_PARTY[0], MON_DATA_PP_BONUSES, &bonuses);
        OPPONENT(SPECIES_WOBBUFFET) { MovesWithPP({MOVE_GROWL, 0}, {MOVE_ROCK_BLAST, 0}, {MOVE_RAPID_SPIN, 0}, {MOVE_RECOVER, 0}); }
        SetMonData(&OPPONENT_PARTY[0], MON_DATA_PP_BONUSES, &bonuses);
    } WHEN {
        TURN { USE_ITEM(player, ITEM_MAX_ELIXIR, partyIndex: 0); USE_ITEM(opponent, ITEM_MAX_ELIXIR, partyIndex: 0); }
    } THEN {
        const u32 expected[] = {61, 40, 32, 8};
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        {
            EXPECT_EQ(player->pp[slot], expected[slot]);
            EXPECT_EQ(opponent->pp[slot], expected[slot]);
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_PP1 + slot), expected[slot]);
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_PP1 + slot), expected[slot]);
        }
    }
}

SINGLE_BATTLE_TEST("Polished move PP restored on the bench survives switching")
{
    u32 bonuses = 0xff;
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { MovesWithPP({MOVE_SPLASH, 10}); }
        PLAYER(SPECIES_CHANSEY) { MovesWithPP({MOVE_GROWL, 0}, {MOVE_ROCK_BLAST, 0}); }
        SetMonData(&PLAYER_PARTY[1], MON_DATA_PP_BONUSES, &bonuses);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { USE_ITEM(player, ITEM_MAX_ETHER, partyIndex: 1, move: MOVE_ROCK_BLAST); }
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(player->pp[0], 0);
        EXPECT_EQ(player->pp[1], 40);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_PP2), 40);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_PP1), 10);
    }
}

DOUBLE_BATTLE_TEST("Polished move PP restoration updates the targeted partner only")
{
    u32 bonuses = 0xff;
    GIVEN {
        WITH_CONFIG(B_POLISHED_MOVE_PP, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { MovesWithPP({MOVE_SPLASH, 10}); }
        PLAYER(SPECIES_CHANSEY) { MovesWithPP({MOVE_GROWL, 0}, {MOVE_SPLASH, 10}); }
        SetMonData(&PLAYER_PARTY[1], MON_DATA_PP_BONUSES, &bonuses);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { USE_ITEM(playerLeft, ITEM_MAX_ETHER, partyIndex: 1, move: MOVE_GROWL); MOVE(playerRight, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(playerRight->pp[0], 61);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_PP1), 61);
        EXPECT_EQ(playerLeft->pp[0], 10);
    }
}
