#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Mirror ledger checks a newly stolen herb against current stat caps")
{
    u32 capped;
    PARAMETRIZE { capped = 0; }
    PARAMETRIZE { capped = 1; }
    PARAMETRIZE { capped = 2; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CONTRARY); Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKPOCKET); MaxHP(1600); HP(1600); }
    } WHEN {
        if (capped >= 1) {
            TURN { MOVE(opponent, MOVE_IRON_DEFENSE); }
            TURN { MOVE(opponent, MOVE_IRON_DEFENSE); }
            TURN { MOVE(opponent, MOVE_IRON_DEFENSE); }
        }
        if (capped == 2) {
            TURN { MOVE(opponent, MOVE_AMNESIA); }
            TURN { MOVE(opponent, MOVE_AMNESIA); }
            TURN { MOVE(opponent, MOVE_AMNESIA); }
        }
        TURN { SWITCH(player, 1); }
        TURN { MOVE(player, MOVE_CLOSE_COMBAT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CLOSE_COMBAT, player);
        ABILITY_POPUP(opponent, ABILITY_PICKPOCKET);
        if (B_POLISHED_MOVE_BALANCE && capped < 2) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        else { NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent); }
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_DEF], capped >= 1 ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + !!B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(opponent->statStages[STAT_SPDEF], capped == 2 ? MAX_STAT_STAGE : DEFAULT_STAT_STAGE + !!B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(opponent->item, B_POLISHED_MOVE_BALANCE && capped < 2 ? ITEM_NONE : ITEM_MIRROR_HERB);
    }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger discards itemless checkpoints before a later Trick")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_TRICK); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRICK, player);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
    } THEN {
        EXPECT_EQ(player->item, ITEM_MIRROR_HERB);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger copies only fresh boosts after acquiring a herb")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_TRICK); }
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 4);
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger clears failed capped copies before the holder has room again")
{
    enum Ability ability;
    PARAMETRIZE { ability = ABILITY_NONE; }
    PARAMETRIZE { ability = ABILITY_CONTRARY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(opponent, MOVE_CHARM); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (ability == ABILITY_CONTRARY ? -4 : 4));
        EXPECT_EQ(player->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
        EXPECT_EQ(player->item, ITEM_MIRROR_HERB);
    }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger applies a new boost after an earlier capped copy failed")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(opponent, MOVE_CHARM); }
        TURN { MOVE(opponent, MOVE_CHARM); }
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 4);
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger clears suppressed item copies before Skill Swap removes Klutz")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KLUTZ); Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_INNER_FOCUS); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_SKILL_SWAP); }
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger records actual increments before the copying holder's Simple")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SIMPLE); Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_INNER_FOCUS); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
        TURN { MOVE(opponent, MOVE_HOWL); }
        TURN { SWITCH(player, 1); }
        TURN { MOVE(opponent, MOVE_SWORDS_DANCE); }
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ATK], MAX_STAT_STAGE);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger retains a Sheer Force skipped berry boost until end turn")
{
    GIVEN {
        PLAYER(SPECIES_NIDOKING) { Ability(ABILITY_SHEER_FORCE); Item(ITEM_MIRROR_HERB); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1600); HP(420); SpDefense(100); Item(ITEM_LIECHI_BERRY); Speed(100); }
    } WHEN { TURN { MOVE(player, MOVE_FLAMETHROWER); } }
    SCENE {
        HP_BAR(opponent);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, opponent);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger prevents copies from feeding the opposing herb")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_MIRROR_HERB); }
    } WHEN { TURN { MOVE(player, MOVE_SHELL_SMASH); } }
    THEN {
        EXPECT_EQ(player->item, ITEM_MIRROR_HERB);
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger copies accuracy and evasion with Contrary")
{
    enum Move move;
    enum Stat stat;
    PARAMETRIZE { move = MOVE_DOUBLE_TEAM; stat = STAT_EVASION; }
    PARAMETRIZE { move = MOVE_HONE_CLAWS; stat = STAT_ACC; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CONTRARY); Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(opponent, move); } }
    THEN { EXPECT_EQ(player->statStages[stat], DEFAULT_STAT_STAGE - 1); }
}

SINGLE_BATTLE_TEST("Polished Mirror ledger leaves native Opportunist copying independent")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_DOWNLOAD); Item(ITEM_MIRROR_HERB); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_OPPORTUNIST); Defense(100); SpDefense(200); }
    } WHEN { TURN {} }
    THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(player->item, ITEM_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished Mirror ledger leaves native Pickpocket eligibility in doubles")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_CONTRARY); Item(ITEM_MIRROR_HERB); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PICKPOCKET); MaxHP(1600); HP(1600); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN { TURN { MOVE(playerLeft, MOVE_CLOSE_COMBAT, target: opponentLeft); } }
    THEN {
        EXPECT_EQ(opponentLeft->item, ITEM_MIRROR_HERB);
        EXPECT_EQ(opponentLeft->statStages[STAT_DEF], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponentLeft->statStages[STAT_SPDEF], DEFAULT_STAT_STAGE);
    }
}
