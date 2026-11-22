#include "global.h"
#include "test/battle.h"
#include "item.h"

WILD_BATTLE_TEST("Polished Suite 2 wild Thief holds stolen items when empty and bags them when already holding")
{
    enum Item held = ITEM_NONE;
    PARAMETRIZE { held = ITEM_NONE; }
    PARAMETRIZE { held = ITEM_POTION; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(held); Moves(MOVE_THIEF); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_HYPER_POTION); }
    } WHEN {
        TURN { MOVE(player, MOVE_THIEF); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->item, held == ITEM_NONE ? ITEM_HYPER_POTION : held);
            EXPECT_EQ(opponent->item, ITEM_NONE);
            EXPECT_EQ(CheckBagHasItem(ITEM_HYPER_POTION, 1), held != ITEM_NONE);
            EXPECT_EQ(GetMonData(GetBattlerMon(B_BATTLER_0), MON_DATA_HELD_ITEM), player->item);
        }
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Thief bypasses Sticky Hold only after the holder faints")
{
    u32 hp = 1000;
    PARAMETRIZE { hp = 1000; }
    PARAMETRIZE { hp = 1; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_THIEF); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STICKY_HOLD); MaxHP(1000); HP(hp); Item(ITEM_POTION); }
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(player, MOVE_THIEF); if (hp == 1) SEND_OUT(opponent, 1); }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(player->item, hp == 1 ? ITEM_POTION : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 trainer Thief cannot take a second held item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_POTION); Moves(MOVE_THIEF); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_HYPER_POTION); }
    } WHEN {
        TURN { MOVE(player, MOVE_THIEF); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POTION);
        EXPECT_EQ(opponent->item, ITEM_HYPER_POTION);
    }
}

WILD_BATTLE_TEST("Polished Suite 2 a wild opponent cannot use Thief to take the players item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_POTION); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_THIEF); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_THIEF); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POTION);
        EXPECT_EQ(opponent->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Knock Off removes the pending healing berry before its HP trigger")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Attack(100); Moves(MOVE_KNOCK_OFF); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(400); HP(250); Defense(100); Item(ITEM_SITRUS_BERRY); }
    } WHEN {
        TURN { MOVE(player, MOVE_KNOCK_OFF); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_KNOCK_OFF, player);
        HP_BAR(opponent);
        if (B_POLISHED_MOVE_BALANCE) NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, opponent);
    } THEN {
        EXPECT_EQ(opponent->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(GetBattlerPartyState(B_BATTLER_1)->usedHeldItem, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Bug Bite reserves a healing berry for its attacker")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Attack(100); MaxHP(400); HP(200); Moves(MOVE_BUG_BITE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(400); HP(250); Defense(100); Item(ITEM_SITRUS_BERRY); }
    } WHEN {
        TURN { MOVE(player, MOVE_BUG_BITE); }
    } THEN {
        EXPECT_EQ(opponent->item, ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ(player->hp, 300);
            EXPECT_EQ(GetBattlerPartyState(B_BATTLER_1)->usedHeldItem, ITEM_NONE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Bug Bite cannot eat EV berries or bypass an active Unnerve")
{
    enum Item item = ITEM_POMEG_BERRY;
    enum Ability ability = ABILITY_SOUNDPROOF;
    PARAMETRIZE { item = ITEM_POMEG_BERRY; ability = ABILITY_SOUNDPROOF; }
    PARAMETRIZE { item = ITEM_KELPSY_BERRY; ability = ABILITY_SOUNDPROOF; }
    PARAMETRIZE { item = ITEM_QUALOT_BERRY; ability = ABILITY_SOUNDPROOF; }
    PARAMETRIZE { item = ITEM_HONDEW_BERRY; ability = ABILITY_SOUNDPROOF; }
    PARAMETRIZE { item = ITEM_GREPA_BERRY; ability = ABILITY_SOUNDPROOF; }
    PARAMETRIZE { item = ITEM_TAMATO_BERRY; ability = ABILITY_SOUNDPROOF; }
    PARAMETRIZE { item = ITEM_ORAN_BERRY; ability = ABILITY_UNNERVE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_BUG_BITE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); MaxHP(1000); HP(1000); Item(item); }
    } WHEN {
        TURN { MOVE(player, MOVE_BUG_BITE); }
    } THEN {
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? item : ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Bug Bite cannot bypass Unnerve with Mold Breaker")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_MOLD_BREAKER); MaxHP(400); HP(200); Moves(MOVE_BUG_BITE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_UNNERVE); MaxHP(1000); HP(1000); Item(ITEM_SITRUS_BERRY); }
    } WHEN {
        TURN { MOVE(player, MOVE_BUG_BITE); }
    } THEN {
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? ITEM_SITRUS_BERRY : ITEM_NONE);
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(player->hp, 200);
    }
}

SINGLE_BATTLE_TEST("Polished Suite 2 Bug Bite arms Cud Chew only after a successful berry effect")
{
    u32 turns = 1;
    PARAMETRIZE { turns = 1; }
    PARAMETRIZE { turns = 3; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CUD_CHEW); MaxHP(400); HP(100); Moves(MOVE_BUG_BITE, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Item(ITEM_SITRUS_BERRY); }
    } WHEN {
        TURN { MOVE(player, MOVE_BUG_BITE); }
        if (turns == 3) {
            TURN { MOVE(player, MOVE_SPLASH); }
            TURN { MOVE(player, MOVE_SPLASH); }
        }
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) {
            EXPECT_EQ((u32)player->volatiles.polishedCudChewBerry, turns == 1 ? ITEM_SITRUS_BERRY : ITEM_NONE);
            EXPECT_EQ(player->hp, turns == 1 ? 200 : 300);
        }
    }
}

WILD_BATTLE_TEST("Polished Suite 2 wild Pickpocket bags a stolen item while already holding one")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKPOCKET); Item(ITEM_POTION); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_HYPER_POTION); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_POTION);
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_HYPER_POTION);
        EXPECT_EQ(CheckBagHasItem(ITEM_HYPER_POTION, 1), B_POLISHED_MOVE_BALANCE);
    }
}

WILD_BATTLE_TEST("Polished Suite 2 theft eligibility rejects a full bag without removing the opponents item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_POTION); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_HYPER_POTION); }
    } WHEN {
        TURN {}
    } THEN {
        // Production eligibility boundary: fill the live pocket through its
        // public API, then ask the same predicate used by Thief/Pickpocket.
        if (B_POLISHED_MOVE_BALANCE) {
            while (AddBagItem(ITEM_HYPER_POTION, MAX_BAG_ITEM_CAPACITY)) {}
            EXPECT_EQ(CheckBagHasSpace(ITEM_HYPER_POTION, 1), FALSE);
            EXPECT_EQ(CanStealItem(B_BATTLER_0, B_BATTLER_1, ITEM_HYPER_POTION), FALSE);
            EXPECT_EQ(opponent->item, ITEM_HYPER_POTION);
        }
    }
}
