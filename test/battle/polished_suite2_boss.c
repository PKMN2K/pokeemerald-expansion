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

SINGLE_BATTLE_TEST("Polished Suite 2 Armor Suit is essential only for either party Armored Mewtwo form")
{
    enum Species species = SPECIES_MEWTWO;
    enum Move move = MOVE_THIEF;
    bool32 enemy = FALSE, armoredUser = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 role = 0; role < 2; role++)
            for (u32 form = 0; form < 3; form++)
                for (u32 command = 0; command < 3; command++)
                    PARAMETRIZE {
                        enemy = side; armoredUser = role;
                        species = form == 0 ? SPECIES_MEWTWO : form == 1 ? SPECIES_MEWTWO_ARMORED : SPECIES_WOBBUFFET;
                        move = command == 0 ? MOVE_THIEF : command == 1 ? MOVE_KNOCK_OFF : MOVE_TRICK;
                    }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    bool32 protected = B_POLISHED_MOVE_BALANCE && species == SPECIES_MEWTWO_ARMORED;
    GIVEN {
        PLAYER(enemy != armoredUser ? species : SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); MaxHP(4000); HP(4000);
            if (enemy) Item(ITEM_POLISHED_ARMOR_SUIT);
        }
        OPPONENT(enemy == armoredUser ? species : SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); MaxHP(4000); HP(4000);
            if (!enemy) Item(ITEM_POLISHED_ARMOR_SUIT);
        }
    } WHEN {
        TURN { MOVE(user, move); }
    } THEN {
        EXPECT_EQ(target->item, protected ? ITEM_POLISHED_ARMOR_SUIT : ITEM_NONE);
        EXPECT_EQ(user->item, !protected && move != MOVE_KNOCK_OFF ? ITEM_POLISHED_ARMOR_SUIT : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Armor Suit eligibility retains the party form after Transform")
{
    bool32 enemy = FALSE, armoredUser = FALSE, armored = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 role = 0; role < 2; role++)
            for (u32 form = 0; form < 2; form++)
                PARAMETRIZE { enemy = side; armoredUser = role; armored = form; }
    enum Species species = armored ? SPECIES_MEWTWO_ARMORED : SPECIES_MEWTWO;
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    struct BattlePokemon *transformed = armoredUser ? user : target;
    struct BattlePokemon *other = armoredUser ? target : user;
    bool32 protected = B_POLISHED_MOVE_BALANCE && armored;
    GIVEN {
        PLAYER(enemy != armoredUser ? species : SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); MaxHP(4000); HP(4000); Speed(100);
            Moves(MOVE_TRANSFORM, MOVE_THIEF, MOVE_SPLASH);
            if (enemy) Item(ITEM_POLISHED_ARMOR_SUIT);
        }
        OPPONENT(enemy == armoredUser ? species : SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); MaxHP(4000); HP(4000); Speed(50);
            Moves(MOVE_TRANSFORM, MOVE_THIEF, MOVE_SPLASH);
            if (!enemy) Item(ITEM_POLISHED_ARMOR_SUIT);
        }
    } WHEN {
        TURN { MOVE(transformed, MOVE_TRANSFORM); MOVE(other, MOVE_SPLASH); }
        TURN { MOVE(user, MOVE_THIEF); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, transformed);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_THIEF, user);
    } THEN {
        EXPECT_EQ(transformed->species, SPECIES_WOBBUFFET);
        EXPECT_EQ(GetMonData(GetBattlerMon(transformed == player ? B_BATTLER_0 : B_BATTLER_1), MON_DATA_SPECIES), species);
        EXPECT_EQ(target->item, protected ? ITEM_POLISHED_ARMOR_SUIT : ITEM_NONE);
        EXPECT_EQ(user->item, protected ? ITEM_NONE : ITEM_POLISHED_ARMOR_SUIT);
    }
}
