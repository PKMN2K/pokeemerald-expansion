#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished medicine updates party and live happiness on both battle sides")
{
    enum Item item;
    u32 penalty, heal = 0, status = STATUS1_NONE;
    bool32 source = TRUE;
    PARAMETRIZE { item = ITEM_ENERGY_POWDER; penalty = 10; heal = 60; }
    PARAMETRIZE { item = ITEM_ENERGY_ROOT; penalty = 15; heal = 120; }
    PARAMETRIZE { item = ITEM_HEAL_POWDER; penalty = 10; status = STATUS1_PARALYSIS; }
    PARAMETRIZE { item = ITEM_ENERGY_POWDER; penalty = 0; heal = 60; source = FALSE; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, source);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); MaxHP(401); Friendship(160); Status1(status); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(401); Friendship(160); Status1(status); }
    } WHEN {
        TURN { USE_ITEM(player, item, partyIndex: 0); USE_ITEM(opponent, item, partyIndex: 0); }
    } SCENE {
        if (heal)
        {
            HP_BAR(player, damage: -heal);
            HP_BAR(opponent, damage: -heal);
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_FRIENDSHIP), 160 - penalty);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_FRIENDSHIP), 160 - penalty);
        EXPECT_EQ(player->friendship, 160 - penalty);
        EXPECT_EQ(opponent->friendship, 160 - penalty);
        EXPECT_EQ(player->status1, STATUS1_NONE);
        EXPECT_EQ(opponent->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished medicine does not penalize unsuccessful battle effects")
{
    enum Item item;
    PARAMETRIZE { item = ITEM_ENERGY_POWDER; }
    PARAMETRIZE { item = ITEM_ENERGY_ROOT; }
    PARAMETRIZE { item = ITEM_HEAL_POWDER; }
    PARAMETRIZE { item = ITEM_REVIVAL_HERB; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { HP(401); MaxHP(401); Friendship(160); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(401); MaxHP(401); Friendship(160); }
    } WHEN {
        TURN { USE_ITEM(player, item, partyIndex: 0); USE_ITEM(opponent, item, partyIndex: 0); }
    } SCENE {
        NONE_OF { HP_BAR(player); HP_BAR(opponent); }
    } THEN {
        EXPECT_EQ(player->friendship, 160);
        EXPECT_EQ(opponent->friendship, 160);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_FRIENDSHIP), 160);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_FRIENDSHIP), 160);
    }
}

SINGLE_BATTLE_TEST("Polished medicine updates only the treated bench member and survives switching")
{
    enum Item item;
    u32 hp, friendship, status = STATUS1_NONE;
    PARAMETRIZE { item = ITEM_ENERGY_POWDER; hp = 1; friendship = 150; }
    PARAMETRIZE { item = ITEM_ENERGY_ROOT; hp = 1; friendship = 145; }
    PARAMETRIZE { item = ITEM_HEAL_POWDER; hp = 1; friendship = 150; status = STATUS1_PARALYSIS; }
    PARAMETRIZE { item = ITEM_REVIVAL_HERB; hp = 0; friendship = 140; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(200); }
        PLAYER(SPECIES_CHANSEY) { HP(hp); MaxHP(401); Friendship(160); Status1(status); }
        OPPONENT(SPECIES_WOBBUFFET) { Friendship(200); }
    } WHEN {
        TURN { USE_ITEM(player, item, partyIndex: 1); }
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_FRIENDSHIP), 200);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_FRIENDSHIP), friendship);
        EXPECT_EQ(player->friendship, friendship);
        EXPECT_EQ(player->status1, STATUS1_NONE);
        EXPECT_EQ(opponent->friendship, 200);
    }
}

DOUBLE_BATTLE_TEST("Polished medicine synchronizes the targeted partner without changing the item user")
{
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Friendship(200); }
        PLAYER(SPECIES_CHANSEY) { HP(1); MaxHP(401); Friendship(160); }
        OPPONENT(SPECIES_WOBBUFFET) { Friendship(200); }
        OPPONENT(SPECIES_CHANSEY) { HP(1); MaxHP(401); Friendship(160); }
    } WHEN {
        TURN {
            USE_ITEM(playerLeft, ITEM_ENERGY_ROOT, partyIndex: 1);
            USE_ITEM(opponentLeft, ITEM_ENERGY_ROOT, partyIndex: 1);
        }
    } SCENE {
        HP_BAR(playerRight, damage: -120);
        HP_BAR(opponentRight, damage: -120);
    } THEN {
        EXPECT_EQ(playerLeft->friendship, 200);
        EXPECT_EQ(opponentLeft->friendship, 200);
        EXPECT_EQ(playerRight->friendship, 145);
        EXPECT_EQ(opponentRight->friendship, 145);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_FRIENDSHIP), 145);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_FRIENDSHIP), 145);
    }
}
