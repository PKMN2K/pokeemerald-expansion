#include "global.h"
#include "test/battle.h"
#include "battle_util.h"

SINGLE_BATTLE_TEST("Polished recovery Pickup can recover an item consumed on an earlier turn")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(player->item, B_POLISHED_MOVE_BALANCE ? ITEM_ORAN_BERRY : ITEM_NONE);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_1)->usedHeldItem, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_ORAN_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Pickup reads the active opponent party slot after switching")
{
    bool32 returnHolder = FALSE;
    PARAMETRIZE { returnHolder = FALSE; }
    PARAMETRIZE { returnHolder = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        TURN { SWITCH(player, 1); SWITCH(opponent, 1); }
        if (returnHolder) TURN { SWITCH(opponent, 0); }
    } THEN {
        EXPECT_EQ(player->item, B_POLISHED_MOVE_BALANCE && returnHolder ? ITEM_ORAN_BERRY : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Pickup does not overwrite a held item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); Item(ITEM_POKE_BALL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POKE_BALL);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_1)->usedHeldItem, ITEM_ORAN_BERRY);
    }
}

WILD_BATTLE_TEST("Polished recovery wild opponents cannot use Pickup")
{
    GIVEN {
        WITH_CONFIG(B_PICKUP_WILD, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_WHITE_HERB); MaxHP(400); HP(199); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_TAIL_WHIP); }
    } THEN {
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_WHITE_HERB);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_0)->usedHeldItem, B_POLISHED_MOVE_BALANCE ? ITEM_WHITE_HERB : ITEM_NONE);
    }
}

WILD_BATTLE_TEST("Polished recovery player Pickup cannot recover its own consumed item")
{
    GIVEN {
        WITH_CONFIG(B_PICKUP_WILD, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); Item(ITEM_WHITE_HERB); MaxHP(400); HP(199); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_TAIL_WHIP); }
    } THEN {
        EXPECT_EQ(player->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_WHITE_HERB);
    }
}

WILD_BATTLE_TEST("Polished recovery player Pickup can recover the wild opponent item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_WHITE_HERB); MaxHP(400); HP(199); }
    } WHEN {
        TURN { MOVE(player, MOVE_TAIL_WHIP); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_WHITE_HERB);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_1)->usedHeldItem, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Pickup can recover a consumed White Herb")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_WHITE_HERB); }
    } WHEN {
        TURN { MOVE(player, MOVE_TAIL_WHIP); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_WHITE_HERB);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_1)->usedHeldItem, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery removed and force-eaten items do not create Pickup history")
{
    enum Move move = MOVE_KNOCK_OFF;
    enum Item item = ITEM_ORAN_BERRY;
    PARAMETRIZE { move = MOVE_KNOCK_OFF; item = ITEM_ORAN_BERRY; }
    PARAMETRIZE { move = MOVE_BUG_BITE; item = ITEM_ORAN_BERRY; }
    PARAMETRIZE { move = MOVE_TACKLE; item = ITEM_AIR_BALLOON; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(item); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, move); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_1)->usedHeldItem, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Harvest preserves a consumption-earned Unburden flag after Transform")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_HARVEST); Speed(150); Moves(MOVE_SPLASH, MOVE_SUNNY_DAY); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SUNNY_DAY); }
        TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
        ABILITY_POPUP(player, ABILITY_HARVEST);
    } THEN {
        EXPECT_EQ(player->item, ITEM_ORAN_BERRY);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_0)->usedHeldItem, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Pickup preserves an earned Unburden flag after Transform")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); Item(ITEM_WHITE_HERB); Speed(150); Moves(MOVE_SUPERPOWER, MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, MOVE_SUPERPOWER); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, player);
        ABILITY_POPUP(player, ABILITY_PICKUP);
    } THEN {
        EXPECT_EQ(player->item, ITEM_WHITE_HERB);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_1)->usedHeldItem, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Harvest only restores berries and never replaces a held item")
{
    bool32 nonBerry = FALSE;
    PARAMETRIZE { nonBerry = FALSE; }
    PARAMETRIZE { nonBerry = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_HARVEST); Item(nonBerry ? ITEM_WHITE_HERB : ITEM_POKE_BALL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY); MOVE(opponent, MOVE_TAIL_WHIP); }
    } THEN {
        EXPECT_EQ(player->item, nonBerry ? ITEM_NONE : ITEM_POKE_BALL);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Harvest has a half chance outside sun")
{
    PASSES_RANDOMLY(1, 2, RNG_HARVEST);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_HARVEST); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_HARVEST);
    } THEN {
        EXPECT_EQ(player->item, ITEM_ORAN_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Harvest is guaranteed in effective sun")
{
    PASSES_RANDOMLY(1, 1, RNG_HARVEST);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_HARVEST); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_HARVEST);
    } THEN {
        EXPECT_EQ(player->item, ITEM_ORAN_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Cloud Nine removes the guaranteed Harvest chance")
{
    PASSES_RANDOMLY(1, 2, RNG_HARVEST);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_HARVEST); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); }
        OPPONENT(SPECIES_GOLDUCK) { Ability(ABILITY_CLOUD_NINE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_HARVEST);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Neutralizing Gas blocks Harvest and Pickup")
{
    enum Ability ability = ABILITY_HARVEST;
    PARAMETRIZE { ability = ABILITY_HARVEST; }
    PARAMETRIZE { ability = ABILITY_PICKUP; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Item(ability == ABILITY_HARVEST ? ITEM_ORAN_BERRY : ITEM_NONE); MaxHP(400); HP(199); }
        OPPONENT(SPECIES_WEEZING) { Ability(ABILITY_NEUTRALIZING_GAS); Item(ability == ABILITY_PICKUP ? ITEM_ORAN_BERRY : ITEM_NONE); MaxHP(400); HP(199); }
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(GetBattlerPartyState(ability == ABILITY_HARVEST ? B_BATTLER_0 : B_BATTLER_1)->usedHeldItem, ITEM_ORAN_BERRY);
    }
}

DOUBLE_BATTLE_TEST("Polished recovery doubles retain the native Pickup freshness restriction")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN {}
        TURN { SWITCH(playerLeft, 2); }
    } THEN {
        EXPECT_EQ(playerLeft->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery active Terastallization retains native Pickup freshness")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPLASH, gimmick: GIMMICK_TERA); MOVE(player, MOVE_SPLASH); }
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery optional Recycle keeps native Unburden reset")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_UNBURDEN); Item(ITEM_ORAN_BERRY); MaxHP(400); HP(199); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_RECYCLE); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_ORAN_BERRY);
        EXPECT_EQ((u32)player->volatiles.unburdenActive, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery Harvest clears history before slower Pickup and defers reconsumption")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_HARVEST); Item(ITEM_ORAN_BERRY); MaxHP(1000); HP(200); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY); MOVE(opponent, MOVE_SCRATCH); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_HARVEST);
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(opponent, ABILITY_PICKUP);
        else ABILITY_POPUP(opponent, ABILITY_PICKUP);
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_ORAN_BERRY);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_0)->usedHeldItem, B_POLISHED_MOVE_BALANCE ? ITEM_ORAN_BERRY : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished recovery both Pickup users consume each opponent history exactly once")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); Item(ITEM_WHITE_HERB); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKUP); Item(ITEM_WHITE_HERB); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_TAIL_WHIP); MOVE(opponent, MOVE_TAIL_WHIP); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_PICKUP);
        ABILITY_POPUP(opponent, ABILITY_PICKUP);
    } THEN {
        EXPECT_EQ(player->item, ITEM_WHITE_HERB);
        EXPECT_EQ(opponent->item, ITEM_WHITE_HERB);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_0)->usedHeldItem, ITEM_NONE);
        EXPECT_EQ(GetBattlerPartyState(B_BATTLER_1)->usedHeldItem, ITEM_NONE);
    }
}
