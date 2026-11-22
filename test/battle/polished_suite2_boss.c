#include "global.h"
#include "test/battle.h"
#include "constants/opponents.h"
#include "polished_trainers.h"
#include "battle_setup.h"
#include "event_data.h"
#include "constants/vars.h"

SINGLE_BATTLE_TEST("Polished Suite 2 Armored Mewtwo opening boost precedes Mirror Herb and entry abilities")
{
    u32 trainer = TRAINER_POLISHED_GIOVANNI1;
    PARAMETRIZE { trainer = TRAINER_POLISHED_GIOVANNI1; }
    PARAMETRIZE { trainer = TRAINER_POLISHED_FALKNER; }
    GIVEN {
        gBattleTestRunnerState->data.recordedBattle.opponentA = trainer;
        PLAYER(SPECIES_EKANS) { Ability(ABILITY_INTIMIDATE); Item(ITEM_MIRROR_HERB); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_MEWTWO_ARMORED) { Ability(ABILITY_INSOMNIA); Item(ITEM_POLISHED_ARMOR_SUIT); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        bool32 boost = B_POLISHED_MOVE_BALANCE && trainer == TRAINER_POLISHED_GIOVANNI1;
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + boost - 1);
        EXPECT_EQ(player->item, boost ? ITEM_NONE : ITEM_MIRROR_HERB);
        for (u32 stat = STAT_ATK; stat <= STAT_SPDEF; stat++) {
            EXPECT_EQ(player->statStages[stat], DEFAULT_STAT_STAGE + boost);
            if (stat != STAT_ATK) EXPECT_EQ(opponent->statStages[stat], DEFAULT_STAT_STAGE + boost);
        }
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Armored Mewtwo boost is trainer specific and never repeats on reentry")
{
    u32 trainer = TRAINER_POLISHED_GIOVANNI1;
    PARAMETRIZE { trainer = TRAINER_POLISHED_GIOVANNI1; }
    PARAMETRIZE { trainer = TRAINER_POLISHED_FALKNER; }
    GIVEN {
        gBattleTestRunnerState->data.recordedBattle.opponentA = trainer;
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SOUNDPROOF); }
        OPPONENT(SPECIES_MEWTWO_ARMORED) { Ability(ABILITY_INSOMNIA); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_SOUNDPROOF); }
    } WHEN {
        TURN { SWITCH(opponent, 1); }
        TURN { SWITCH(opponent, 0); }
    } THEN {
        for (u32 stat = STAT_ATK; stat <= STAT_SPDEF; stat++)
            EXPECT_EQ(opponent->statStages[stat], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Armor Suit is essential when either battler is Mewtwo")
{
    enum Species species = SPECIES_MEWTWO;
    PARAMETRIZE { species = SPECIES_MEWTWO; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; }
    GIVEN {
        PLAYER(species) { Moves(MOVE_THIEF); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Item(ITEM_POLISHED_ARMOR_SUIT); }
    } WHEN {
        TURN { MOVE(player, MOVE_THIEF); }
    } THEN {
        EXPECT_EQ(player->item, B_POLISHED_MOVE_BALANCE && species == SPECIES_MEWTWO ? ITEM_NONE : ITEM_POLISHED_ARMOR_SUIT);
    }
}
