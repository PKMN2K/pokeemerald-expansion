#include "global.h"
#include "test/battle.h"
#include "battle_util.h"

SINGLE_BATTLE_TEST("Polished entry closure Trace immediately applies copied Magma Armor only to freeze")
{
    struct BattlePokemon *holder = player;
    u32 status = STATUS1_FREEZE;
    for (u32 side = 0; side < 2; side++) {
        PARAMETRIZE { holder = side ? opponent : player; status = STATUS1_FREEZE; }
        PARAMETRIZE { holder = side ? opponent : player; status = STATUS1_BURN; }
        PARAMETRIZE { holder = side ? opponent : player; status = STATUS1_NONE; }
    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_TRACE : ABILITY_MAGMA_ARMOR); Status1(holder == player ? status : STATUS1_NONE); Speed(holder == player ? 1000 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_TRACE : ABILITY_MAGMA_ARMOR); Status1(holder == opponent ? status : STATUS1_NONE); Speed(holder == opponent ? 1000 : 50); }
    } WHEN { TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); } }
    SCENE {
        ABILITY_POPUP(holder, ABILITY_TRACE);
        if (status == STATUS1_FREEZE) ABILITY_POPUP(holder, ABILITY_MAGMA_ARMOR);
        else NOT ABILITY_POPUP(holder, ABILITY_MAGMA_ARMOR);
    } THEN {
        EXPECT_EQ(holder->ability, ABILITY_MAGMA_ARMOR);
        EXPECT_EQ(holder->status1, status == STATUS1_FREEZE ? STATUS1_NONE : status);
    }
}

SINGLE_BATTLE_TEST("Polished entry closure Trace retries after each source blacklist ability leaves")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability blocked = ABILITY_TRACE;
    static const enum Ability blockedAbilities[] = { ABILITY_TRACE, ABILITY_IMPOSTER, ABILITY_NEUTRALIZING_GAS };
    for (u32 side = 0; side < 2; side++)
        for (u32 ability = 0; ability < ARRAY_COUNT(blockedAbilities); ability++)
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; blocked = blockedAbilities[ability]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_TRACE : blocked); Speed(holder == player ? 1000 : 50); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_TRACE : blocked); Speed(holder == opponent ? 1000 : 50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); Speed(50); }
    } WHEN { TURN { SWITCH(other, 1); MOVE(holder, MOVE_SPLASH); } }
    SCENE { ABILITY_POPUP(holder, ABILITY_TRACE); }
    THEN {
        EXPECT_EQ(holder->ability, ABILITY_PRESSURE);
        if (blocked == ABILITY_TRACE) {
            // Ability(NONE) selects an empty species slot and falls back to a
            // real ability in the fixture loader. Probe the actual dispatcher
            // directly for this otherwise unrepresentable initial state.
            enum Ability savedHolder = holder->ability, savedOther = other->ability;
            bool32 savedTrace = holder->volatiles.traceActivated;
            holder->ability = ABILITY_TRACE;
            holder->volatiles.traceActivated = FALSE;
            other->ability = ABILITY_NONE;
            EXPECT_EQ(AbilityBattleEffects(ABILITYEFFECT_ON_SWITCHIN, holder - gBattleMons, ABILITY_TRACE, MOVE_SPLASH, TRUE), 0);
            EXPECT_EQ(holder->ability, ABILITY_TRACE);
            EXPECT(!holder->volatiles.traceActivated);
            holder->ability = savedHolder;
            other->ability = savedOther;
            holder->volatiles.traceActivated = savedTrace;
        }
    }
}

SINGLE_BATTLE_TEST("Polished entry closure Magma Armor gained by Skill Swap cures freeze before the pending action")
{
    struct BattlePokemon *holder = player, *other = opponent;
    PARAMETRIZE { holder = player; other = opponent; }
    PARAMETRIZE { holder = opponent; other = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_PRESSURE : ABILITY_MAGMA_ARMOR); Status1(holder == player ? STATUS1_FREEZE : STATUS1_NONE); Speed(holder == player ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_PRESSURE : ABILITY_MAGMA_ARMOR); Status1(holder == opponent ? STATUS1_FREEZE : STATUS1_NONE); Speed(holder == opponent ? 50 : 100); }
    } WHEN { TURN { MOVE(other, MOVE_SKILL_SWAP); MOVE(holder, MOVE_SPLASH); } }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKILL_SWAP, other);
        ABILITY_POPUP(holder, ABILITY_MAGMA_ARMOR);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, holder);
    } THEN {
        EXPECT_EQ(holder->ability, ABILITY_MAGMA_ARMOR);
        EXPECT_EQ(holder->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished entry closure Magma Armor waits for Neutralizing Gas to leave")
{
    struct BattlePokemon *holder = player, *other = opponent;
    PARAMETRIZE { holder = player; other = opponent; }
    PARAMETRIZE { holder = opponent; other = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_MAGMA_ARMOR : ABILITY_NEUTRALIZING_GAS); Status1(holder == player ? STATUS1_FREEZE : STATUS1_NONE); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_MAGMA_ARMOR : ABILITY_NEUTRALIZING_GAS); Status1(holder == opponent ? STATUS1_FREEZE : STATUS1_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); }
    } WHEN { TURN { SWITCH(other, 1); MOVE(holder, MOVE_SPLASH); } }
    SCENE {
        ABILITY_POPUP(other, ABILITY_NEUTRALIZING_GAS);
        NOT ABILITY_POPUP(holder, ABILITY_MAGMA_ARMOR);
        MESSAGE("The effects of the neutralizing gas wore off!");
        ABILITY_POPUP(holder, ABILITY_MAGMA_ARMOR);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, holder);
    } THEN { EXPECT_EQ(holder->status1, STATUS1_NONE); }
}

SINGLE_BATTLE_TEST("Polished entry closure Frisk checks current held items and Gas suppression for either side")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Ability otherAbility = ABILITY_NONE;
    enum Item item = ITEM_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 gas = 0; gas < 2; gas++) {
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; otherAbility = gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE; item = ITEM_NONE; }
            PARAMETRIZE { holder = side ? opponent : player; other = side ? player : opponent; otherAbility = gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE; item = ITEM_LEFTOVERS; }
        }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(holder == player ? ABILITY_FRISK : otherAbility); Item(other == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(holder == opponent ? ABILITY_FRISK : otherAbility); Item(other == opponent ? item : ITEM_NONE); }
    } WHEN { TURN {} }
    SCENE {
        if (item != ITEM_NONE && otherAbility != ABILITY_NEUTRALIZING_GAS) ABILITY_POPUP(holder, ABILITY_FRISK);
        else NOT ABILITY_POPUP(holder, ABILITY_FRISK);
    } THEN { EXPECT_EQ(other->item, item); }
}

SINGLE_BATTLE_TEST("Polished entry closure Frisk acquired after Trick reveals the replacement item")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); Item(ITEM_ORAN_BERRY); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_FRISK); Item(ITEM_LEFTOVERS); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRICK); }
        TURN { MOVE(player, MOVE_SKILL_SWAP); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_FRISK);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRICK, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKILL_SWAP, player);
        ABILITY_POPUP(player, ABILITY_FRISK);
        MESSAGE("The opposing Wobbuffet was frisked, revealing its Oran Berry!");
    } THEN {
        EXPECT_EQ(player->item, ITEM_LEFTOVERS);
        EXPECT_EQ(opponent->item, ITEM_ORAN_BERRY);
    }
}

SINGLE_BATTLE_TEST("Polished entry closure Screen Cleaner removes both sides screens but preserves Mist and Safeguard")
{
    enum Ability otherAbility = ABILITY_NONE;
    PARAMETRIZE { otherAbility = ABILITY_NONE; }
    PARAMETRIZE { otherAbility = ABILITY_NEUTRALIZING_GAS; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SCREEN_CLEANER); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(otherAbility); }
    } WHEN {
        TURN { MOVE(player, MOVE_REFLECT); MOVE(opponent, MOVE_LIGHT_SCREEN); }
        TURN { MOVE(player, MOVE_LIGHT_SCREEN); MOVE(opponent, MOVE_REFLECT); }
        TURN { MOVE(player, MOVE_MIST); MOVE(opponent, MOVE_SAFEGUARD); }
        TURN { SWITCH(player, 1); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        if (otherAbility != ABILITY_NEUTRALIZING_GAS) ABILITY_POPUP(player, ABILITY_SCREEN_CLEANER);
        else NOT ABILITY_POPUP(player, ABILITY_SCREEN_CLEANER);
    } THEN {
        u32 screens = SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN;
        EXPECT_EQ(gSideStatuses[B_SIDE_PLAYER] & screens, otherAbility == ABILITY_NEUTRALIZING_GAS ? screens : 0);
        EXPECT_EQ(gSideStatuses[B_SIDE_OPPONENT] & screens, otherAbility == ABILITY_NEUTRALIZING_GAS ? screens : 0);
        EXPECT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_MIST);
        EXPECT(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_SAFEGUARD);
    }
}

SINGLE_BATTLE_TEST("Polished entry closure Screen Cleaner gained with only Mist and Safeguard stays silent")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SCREEN_CLEANER); }
    } WHEN {
        TURN { MOVE(player, MOVE_MIST); MOVE(opponent, MOVE_SAFEGUARD); }
        TURN { MOVE(player, MOVE_SKILL_SWAP); }
    } SCENE {
        NOT ABILITY_POPUP(opponent, ABILITY_SCREEN_CLEANER);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SKILL_SWAP, player);
        NOT ABILITY_POPUP(player, ABILITY_SCREEN_CLEANER);
    } THEN {
        EXPECT_EQ(player->ability, ABILITY_SCREEN_CLEANER);
        EXPECT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_MIST);
        EXPECT(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_SAFEGUARD);
    }
}
