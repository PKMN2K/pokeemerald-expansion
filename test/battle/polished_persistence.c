#include "global.h"
#include "test/battle.h"
#include "battle_util.h"
#include "battle_gimmick.h"
#include "item.h"

// These fixtures also exercise the production restoration entry on party data
// written by real commands. Terminal cleanup itself precedes THEN; dedicated
// loss/whiteout/full-party capture assertions live in polished_cleanup_closure.c.
SINGLE_BATTLE_TEST("Polished persistence restores removed berries but not eaten berries")
{
    enum Move move = MOVE_KNOCK_OFF;
    PARAMETRIZE { move = MOVE_KNOCK_OFF; }
    PARAMETRIZE { move = MOVE_BUG_BITE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_ORAN_BERRY); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, move); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM),
                  B_POLISHED_MOVE_BALANCE && move == MOVE_KNOCK_OFF ? ITEM_ORAN_BERRY : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished persistence consumes berries permanently and restores nonberries")
{
    enum Item item = ITEM_ORAN_BERRY;
    PARAMETRIZE { item = ITEM_ORAN_BERRY; }
    PARAMETRIZE { item = ITEM_WHITE_HERB; }
    PARAMETRIZE { item = ITEM_AIR_BALLOON; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(item); MaxHP(1000); HP(499); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, item == ITEM_AIR_BALLOON ? MOVE_TACKLE : MOVE_TAIL_WHIP); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM), item == ITEM_ORAN_BERRY ? ITEM_NONE : item);
    }
}

SINGLE_BATTLE_TEST("Polished persistence trainer Trick restores the original berry")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_ORAN_BERRY); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_POTION); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRICK); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POTION);
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM), B_POLISHED_MOVE_BALANCE ? ITEM_ORAN_BERRY : ITEM_NONE);
    }
}

WILD_BATTLE_TEST("Polished persistence wild Trick overwrites even a nonempty backup")
{
    enum Item item = ITEM_NONE;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_POKE_BALL; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(item); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_POTION); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRICK); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POTION);
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM), ITEM_POTION);
    }
}

WILD_BATTLE_TEST("Polished persistence wild Thief retains a held or bagged stolen item")
{
    enum Item item = ITEM_NONE;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_POKE_BALL; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(item); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_POTION); }
    } WHEN {
        TURN { MOVE(player, MOVE_THIEF); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            TryRestoreHeldItems();
            EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM), item == ITEM_NONE ? ITEM_POTION : item);
            EXPECT_EQ(CheckBagHasItem(ITEM_POTION, 1), item != ITEM_NONE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished persistence eating a stolen berry does not erase a different original item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_WHITE_HERB); MaxHP(1000); HP(499); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_ORAN_BERRY); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TAIL_WHIP); }
        TURN { MOVE(player, MOVE_THIEF); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM), ITEM_WHITE_HERB);
    }
}

SINGLE_BATTLE_TEST("Polished persistence Harvest replenishes the backup after consumption")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_HARVEST); Item(ITEM_ORAN_BERRY); MaxHP(1000); HP(499); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY); }
        TURN { MOVE(opponent, MOVE_KNOCK_OFF); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM), B_POLISHED_MOVE_BALANCE ? ITEM_ORAN_BERRY : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished persistence trainer Pickup returns a recovered opponent item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_WHITE_HERB); }
    } WHEN {
        TURN { MOVE(player, MOVE_TAIL_WHIP); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_WHITE_HERB);
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM), ITEM_NONE);
    }
}

WILD_BATTLE_TEST("Polished persistence wild Pickup fills only an empty backup")
{
    enum Item item = ITEM_NONE;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_MENTAL_HERB; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); Item(item); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_WHITE_HERB); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, item == ITEM_NONE ? MOVE_SPLASH : MOVE_ENCORE); }
        TURN { MOVE(player, MOVE_TAIL_WHIP); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->item, ITEM_WHITE_HERB);
            TryRestoreHeldItems();
            EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM), item == ITEM_NONE ? ITEM_WHITE_HERB : item);
        }
    }
}

SINGLE_BATTLE_TEST("Polished persistence restores each party slot including a switched out holder")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_ORAN_BERRY); MaxHP(1000); HP(1000); }
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_PECHA_BERRY); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_KNOCK_OFF); }
        TURN { SWITCH(player, 1); MOVE(opponent, MOVE_KNOCK_OFF); }
    } THEN {
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), B_POLISHED_MOVE_BALANCE ? ITEM_ORAN_BERRY : ITEM_NONE);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HELD_ITEM), B_POLISHED_MOVE_BALANCE ? ITEM_PECHA_BERRY : ITEM_NONE);
    }
}

WILD_BATTLE_TEST("Polished persistence a newly caught party member retains its current item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_POTION); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_MASTER_BALL); }
    } THEN {
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_WOBBUFFET);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HELD_ITEM), ITEM_POTION);
    }
}

SINGLE_BATTLE_TEST("Polished persistence a berry consumed during Dynamax stays consumed after it ends")
{
    u32 boundary = 0;
    PARAMETRIZE { boundary = 0; } // Still active: native restoration control.
    PARAMETRIZE { boundary = 1; } // Three turns expire.
    PARAMETRIZE { boundary = 2; } // Switching ends Dynamax.
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(100); MaxHP(100); Item(ITEM_ORAN_BERRY); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(40); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(100); Speed(100); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, gimmick: GIMMICK_DYNAMAX); MOVE(opponent, MOVE_SEISMIC_TOSS); }
        if (boundary == 1) {
            TURN { MOVE(player, MOVE_TACKLE); }
            TURN { MOVE(player, MOVE_TACKLE); }
        } else if (boundary == 2) {
            TURN { SWITCH(player, 1); }
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_DYNAMAX_GROWTH, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, opponent);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, player);
    } THEN {
        EXPECT_EQ(GetActiveGimmick(B_BATTLER_0), boundary == 0 ? GIMMICK_DYNAMAX : GIMMICK_NONE);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), ITEM_NONE);
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), ITEM_NONE);
    }
}

WILD_BATTLE_TEST("Polished persistence native Pickup during Dynamax retains its acquired item")
{
    u32 boundary = 0;
    PARAMETRIZE { boundary = 0; } // Still active: native restoration control.
    PARAMETRIZE { boundary = 1; } // Three turns expire.
    PARAMETRIZE { boundary = 2; } // Switching ends Dynamax.
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); Speed(100); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); HP(1000); MaxHP(1000); Item(ITEM_WHITE_HERB); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, gimmick: GIMMICK_DYNAMAX); }
        if (boundary == 1) {
            TURN { MOVE(player, MOVE_TACKLE); }
            TURN { MOVE(player, MOVE_TACKLE); }
        } else if (boundary == 2) {
            TURN { SWITCH(player, 1); }
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_DYNAMAX_GROWTH, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MAX_STRIKE, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        ABILITY_POPUP(player, ABILITY_PICKUP);
        MESSAGE("Wobbuffet found one White Herb!");
    } THEN {
        EXPECT_EQ(GetActiveGimmick(B_BATTLER_0), boundary == 0 ? GIMMICK_DYNAMAX : GIMMICK_NONE);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), ITEM_WHITE_HERB);
        TryRestoreHeldItems();
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), ITEM_WHITE_HERB);
    }
}
