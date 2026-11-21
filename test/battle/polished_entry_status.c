#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished entry Toxic Spikes synchronizes on voluntary entry before curing the entrant")
{
    u32 layers = 1;
    enum Item berry = ITEM_NONE;
    PARAMETRIZE { layers = 1; berry = ITEM_NONE; }
    PARAMETRIZE { layers = 2; berry = ITEM_NONE; }
    PARAMETRIZE { layers = 1; berry = ITEM_PECHA_BERRY; }
    PARAMETRIZE { layers = 2; berry = ITEM_LUM_BERRY; }
    u32 status = layers == 1 ? STATUS1_POISON : STATUS1_TOXIC_POISON;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_SYNCHRONIZE); Item(berry); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(160); HP(160); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TOXIC_SPIKES); }
        if (layers == 2) TURN { MOVE(opponent, MOVE_TOXIC_SPIKES); }
        TURN { SWITCH(player, 1); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ABILITY_POPUP(player, ABILITY_SYNCHRONIZE);
            STATUS_ICON(opponent, status);
        } else {
            NOT ABILITY_POPUP(player, ABILITY_SYNCHRONIZE);
        }
        if (berry != ITEM_NONE) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
    } THEN {
        EXPECT_EQ(opponent->status1 & STATUS1_ANY, B_POLISHED_MOVE_BALANCE ? status : STATUS1_NONE);
        EXPECT_EQ(player->status1 & STATUS1_ANY, berry == ITEM_NONE ? status : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished entry forced Toxic Spikes do not trigger Synchronize")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(opponent, MOVE_ROAR); }
    } SCENE { NOT ABILITY_POPUP(player, ABILITY_SYNCHRONIZE); }
    THEN {
        EXPECT_EQ(player->status1 & STATUS1_ANY, STATUS1_POISON);
        EXPECT_EQ(opponent->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished entry Synchronize respects the opposing Substitute")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_SYNCHRONIZE); Item(ITEM_PECHA_BERRY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN { SWITCH(player, 1); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) ABILITY_POPUP(player, ABILITY_SYNCHRONIZE);
        else NOT ABILITY_POPUP(player, ABILITY_SYNCHRONIZE);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
    } THEN {
        EXPECT_EQ(opponent->status1, STATUS1_NONE);
        EXPECT_EQ(player->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished entry forced context clears before a later voluntary switch")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_SYNCHRONIZE); Item(ITEM_PECHA_BERRY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(opponent, MOVE_ROAR); }
        TURN { SWITCH(player, 0); }
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(opponent->status1 & STATUS1_ANY, B_POLISHED_MOVE_BALANCE ? STATUS1_POISON : STATUS1_NONE);
        EXPECT_EQ(player->status1 & STATUS1_ANY, STATUS1_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished entry both replacements finish status reflection and cures before entry abilities")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1); Status1(STATUS1_POISON); }
        PLAYER(SPECIES_ABRA) { Ability(ABILITY_SYNCHRONIZE); Speed(100); Item(ITEM_PECHA_BERRY); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(1); Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_ARCANINE) { Ability(ABILITY_INTIMIDATE); Speed(50); MaxHP(160); HP(160); }
    } WHEN {
        TURN { MOVE(player, MOVE_TOXIC_SPIKES); MOVE(opponent, MOVE_TOXIC_SPIKES); SEND_OUT(player, 1); SEND_OUT(opponent, 1); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ABILITY_POPUP(player, ABILITY_SYNCHRONIZE);
            STATUS_ICON(opponent, STATUS1_POISON);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
            ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        }
    } THEN {
        EXPECT_EQ(player->status1, STATUS1_NONE);
        EXPECT_EQ(opponent->status1 & STATUS1_ANY, STATUS1_POISON);
        EXPECT_EQ(player->hp, 160);
        EXPECT_EQ(opponent->hp, 160);
    }
}

SINGLE_BATTLE_TEST("Polished entry Mold Breaker Roar bypasses Levitate but keeps Magic Guard against Spikes")
{
    enum Ability ability = ABILITY_LEVITATE;
    PARAMETRIZE { ability = ABILITY_LEVITATE; }
    PARAMETRIZE { ability = ABILITY_MAGIC_GUARD; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_CLEFABLE) { Ability(ability); MaxHP(160); HP(160); }
        OPPONENT(SPECIES_PINSIR) { Ability(ABILITY_MOLD_BREAKER); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPIKES); }
        TURN { MOVE(opponent, MOVE_ROAR); }
    } THEN {
        EXPECT_EQ(player->hp, ability == ABILITY_MAGIC_GUARD ? 160 : 140);
    }
}
