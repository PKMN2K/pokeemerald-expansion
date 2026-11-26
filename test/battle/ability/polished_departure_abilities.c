#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished departure Natural Cure uses the effective ability and the departing party slot")
{
    struct BattlePokemon *holder = player;
    enum Ability blocker = ABILITY_NONE;
    u32 status = STATUS1_POISON;
    for (u32 side = 0; side < 2; side++)
        for (u32 gas = 0; gas < 2; gas++) {
            PARAMETRIZE { holder = side ? opponent : player; blocker = gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE; status = STATUS1_POISON; }
            PARAMETRIZE { holder = side ? opponent : player; blocker = gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE; status = STATUS1_BURN; }
            PARAMETRIZE { holder = side ? opponent : player; blocker = gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE; status = STATUS1_PARALYSIS; }
        }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_NATURAL_CURE : blocker); Status1(holder == player ? status : STATUS1_NONE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Status1(STATUS1_POISON); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_NATURAL_CURE : blocker); Status1(holder == opponent ? status : STATUS1_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Status1(STATUS1_POISON); HP(1000); MaxHP(1000); }
    } WHEN { TURN { SWITCH(holder, 1); } }
    THEN {
        u32 side = holder == player ? B_TRAINER_PLAYER : B_TRAINER_OPPONENT_A;
        EXPECT_EQ(GetMonData(&gParties[side][0], MON_DATA_STATUS), blocker == ABILITY_NEUTRALIZING_GAS ? status : STATUS1_NONE);
        EXPECT_EQ(GetMonData(&gParties[side][1], MON_DATA_STATUS), STATUS1_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished departure Natural Cure follows Skill Swap acquisition and loss")
{
    bool32 acquired = FALSE;
    PARAMETRIZE { acquired = FALSE; }
    PARAMETRIZE { acquired = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(acquired ? ABILITY_PRESSURE : ABILITY_NATURAL_CURE); Status1(STATUS1_BURN); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(acquired ? ABILITY_NATURAL_CURE : ABILITY_PRESSURE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SKILL_SWAP); }
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_STATUS), acquired ? STATUS1_NONE : STATUS1_BURN);
    }
}
