#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished entry Pack replaces a slower entrant before its weather ability")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        PLAYER(SPECIES_POLITOED) { Ability(ABILITY_DRIZZLE); Item(ITEM_EJECT_PACK); Speed(50); }
        PLAYER(SPECIES_TYRANITAR) { Ability(ABILITY_SAND_STREAM); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); Speed(100); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_EXPLOSION); SEND_OUT(player, 1); SEND_OUT(opponent, 1); SEND_OUT(player, 2); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, player);
        ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        if (B_POLISHED_MOVE_BALANCE) {
            NOT ABILITY_POPUP(player, ABILITY_DRIZZLE);
        } else {
            ABILITY_POPUP(player, ABILITY_DRIZZLE);
        }
        MESSAGE("Politoed is switched out with the Eject Pack!");
        ABILITY_POPUP(player, ABILITY_SAND_STREAM);
    } THEN { EXPECT_EQ(player->species, SPECIES_TYRANITAR); }
}

SINGLE_BATTLE_TEST("Polished entry both-entry loop revisits the replacement's Intimidate")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        PLAYER(SPECIES_PORYGON) { Ability(ABILITY_DOWNLOAD); Item(ITEM_EJECT_PACK); Speed(50); }
        PLAYER(SPECIES_EKANS) { Ability(ABILITY_INTIMIDATE); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); Speed(100); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_EXPLOSION); SEND_OUT(player, 1); SEND_OUT(opponent, 1); SEND_OUT(player, 2); } }
    THEN { EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE - (B_POLISHED_MOVE_BALANCE ? 2 : 1)); }
}

SINGLE_BATTLE_TEST("Polished entry Intimidate copies Rattled before restoring White Herb")
{
    GIVEN {
        PLAYER(SPECIES_DUNSPARCE) { Ability(ABILITY_RATTLED); Item(ITEM_WHITE_HERB); Speed(50); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); Item(ITEM_MIRROR_HERB); Speed(100); }
    } WHEN { TURN {} }
    SCENE {
        ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        ABILITY_POPUP(player, ABILITY_RATTLED);
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        } else {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished entry Intimidate Pack needs a currently negative stage")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { SWITCH(opponent, 1); if (!B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); }
    } THEN { EXPECT_EQ(player->species, B_POLISHED_MOVE_BALANCE ? SPECIES_WOBBUFFET : SPECIES_CHANSEY); }
}

SINGLE_BATTLE_TEST("Polished entry Intimidate immunity is independent of the native generation")
{
    enum Ability ability;
    PARAMETRIZE { ability = ABILITY_INNER_FOCUS; }
    PARAMETRIZE { ability = ABILITY_OWN_TEMPO; }
    PARAMETRIZE { ability = ABILITY_OBLIVIOUS; }
    PARAMETRIZE { ability = ABILITY_SCRAPPY; }
    GIVEN {
        WITH_CONFIG(B_UPDATED_INTIMIDATE, GEN_7);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); }
    } WHEN { TURN {} }
    THEN { EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE - !B_POLISHED_MOVE_BALANCE); }
}

SINGLE_BATTLE_TEST("Polished entry Intimidate Rattled is independent of the native generation")
{
    GIVEN {
        WITH_CONFIG(B_UPDATED_INTIMIDATE, GEN_7);
        PLAYER(SPECIES_DUNSPARCE) { Ability(ABILITY_RATTLED); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); }
    } WHEN { TURN {} }
    THEN { EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + !!B_POLISHED_MOVE_BALANCE); }
}

SINGLE_BATTLE_TEST("Polished entry Pack chain preserves the surrounding U turn action")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); MaxHP(1600); HP(1600); Speed(50); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_U_TURN); SEND_OUT(player, 1); SEND_OUT(opponent, 1); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_U_TURN, player);
        ABILITY_POPUP(player, ABILITY_INTIMIDATE);
        MESSAGE("The opposing Wobbuffet is switched out with the Eject Pack!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(player->species, SPECIES_ARBOK);
        EXPECT_EQ(opponent->species, SPECIES_CHANSEY);
    }
}

SINGLE_BATTLE_TEST("Polished entry Pack triggered by Skill Swap resumes the calling move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); Item(ITEM_EJECT_PACK); Speed(50); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SKILL_SWAP); SEND_OUT(opponent, 1); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(player->ability, ABILITY_INTIMIDATE);
        EXPECT_EQ(opponent->species, SPECIES_CHANSEY);
    }
}

SINGLE_BATTLE_TEST("Polished entry Gas departure prevents a nested Pack switch")
{
    GIVEN {
        PLAYER(SPECIES_WEEZING) { Ability(ABILITY_NEUTRALIZING_GAS); Item(ITEM_EJECT_PACK); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); }
    } WHEN { TURN { SWITCH(player, 1); } }
    SCENE {
        ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        NOT MESSAGE("Weezing is switched out with the Eject Pack!");
        SEND_IN_MESSAGE("Chansey");
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), ITEM_EJECT_PACK);
    }
}

SINGLE_BATTLE_TEST("Polished entry Pack replacement can faint to hazards before resuming")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); HP(1); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); Speed(50); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPIKES); }
        TURN { SWITCH(opponent, 1); SEND_OUT(player, 1); SEND_OUT(player, 2); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        MESSAGE("Wobbuffet is switched out with the Eject Pack!");
        SEND_IN_MESSAGE("Chansey");
        HP_BAR(player);
        MESSAGE("Chansey fainted!");
        SEND_IN_MESSAGE("Wynaut");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
    } THEN { EXPECT_EQ(player->species, SPECIES_WYNAUT); }
}

SINGLE_BATTLE_TEST("Polished entry chained Intimidate Packs fit both full parties")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        for (u32 i = 1; i < PARTY_SIZE; i++) {
            PLAYER(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); Item(i < PARTY_SIZE - 1 ? ITEM_EJECT_PACK : ITEM_NONE); Speed(100); }
        }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); Speed(50); }
        for (u32 i = 1; i < PARTY_SIZE; i++) {
            OPPONENT(SPECIES_EKANS) { Ability(ABILITY_INTIMIDATE); Item(i < PARTY_SIZE - 1 ? ITEM_EJECT_PACK : ITEM_NONE); Speed(50); }
        }
    } WHEN {
        TURN {
            MOVE(player, MOVE_EXPLOSION); SEND_OUT(player, 1); SEND_OUT(opponent, 1);
            for (u32 i = 2; i < PARTY_SIZE; i++) { SEND_OUT(opponent, i); SEND_OUT(player, i); }
        }
        TURN { MOVE(player, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[B_BATTLER_0], PARTY_SIZE - 1);
        EXPECT_EQ(gBattlerPartyIndexes[B_BATTLER_1], PARTY_SIZE - 1);
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(opponent->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished entry Pack selects current stages in Speed and Trick Room order through Ingrain")
{
    bool32 trickRoom;
    PARAMETRIZE { trickRoom = FALSE; }
    PARAMETRIZE { trickRoom = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_INGRAIN); MOVE(opponent, MOVE_GROWL); }
        TURN { if (trickRoom) MOVE(player, MOVE_TRICK_ROOM); }
        TURN {
            MOVE(player, MOVE_BATON_PASS); SEND_OUT(player, 1);
            if (B_POLISHED_MOVE_BALANCE && !trickRoom) SEND_OUT(player, 2);
            else SEND_OUT(opponent, 1);
        }
    } THEN {
        EXPECT_EQ(player->species, B_POLISHED_MOVE_BALANCE && !trickRoom ? SPECIES_CHANSEY : SPECIES_ARBOK);
        EXPECT_EQ(opponent->species, B_POLISHED_MOVE_BALANCE && !trickRoom ? SPECIES_WOBBUFFET : SPECIES_WYNAUT);
    }
}

SINGLE_BATTLE_TEST("Polished entry Pack with no living reserve stays held")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); }
        PLAYER(SPECIES_CHANSEY) { HP(0); }
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_INTIMIDATE); }
    } WHEN { TURN {} }
    SCENE { NOT MESSAGE("Wobbuffet is switched out with the Eject Pack!"); }
    THEN {
        EXPECT_EQ(player->item, ITEM_EJECT_PACK);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
    }
}
