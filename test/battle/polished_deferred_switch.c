#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished deferred Eject Pack requires a negative stage after a stat drop")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_DEFENSE_CURL); }
        TURN { MOVE(player, MOVE_AMNESIA); }
        TURN { MOVE(player, MOVE_CLOSE_COMBAT); if (!B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(player->species, B_POLISHED_MOVE_BALANCE ? SPECIES_WOBBUFFET : SPECIES_CHANSEY);
        // Native battle cleanup restores the player's consumed held items.
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), ITEM_EJECT_PACK);
    }
}

SINGLE_BATTLE_TEST("Polished deferred Eject Pack checks existing negative stages after Trick")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_TRICK); if (B_POLISHED_MOVE_BALANCE) SEND_OUT(opponent, 1); }
    } THEN {
        EXPECT_EQ(opponent->species, B_POLISHED_MOVE_BALANCE ? SPECIES_CHANSEY : SPECIES_WOBBUFFET);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM), B_POLISHED_MOVE_BALANCE ? ITEM_NONE : ITEM_EJECT_PACK);
    }
}

SINGLE_BATTLE_TEST("Polished deferred U turn reservation precedes Weak Armor Eject Pack")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(200); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_ONIX) { Ability(ABILITY_WEAK_ARMOR); Item(ITEM_EJECT_PACK); MaxHP(1600); HP(1600); Speed(20); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_U_TURN); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(opponent->species, SPECIES_ONIX);
        EXPECT_EQ(opponent->item, ITEM_EJECT_PACK);
    }
}

SINGLE_BATTLE_TEST("Polished deferred an ineligible Button reservation still excludes Eject Pack")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_BUTTON); MaxHP(1600); HP(1600); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_CLOSE_COMBAT); if (!B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(player->species, B_POLISHED_MOVE_BALANCE ? SPECIES_WOBBUFFET : SPECIES_CHANSEY);
        EXPECT_EQ(opponent->item, ITEM_EJECT_BUTTON);
        // Native battle cleanup restores the player's consumed held items.
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM), ITEM_EJECT_PACK);
    }
}

SINGLE_BATTLE_TEST("Polished deferred Button reservation wins over the attacker's Eject Pack")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_BUTTON); MaxHP(1600); HP(1600); Speed(50); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_CLOSE_COMBAT); SEND_OUT(opponent, 1); } }
    THEN { EXPECT_EQ(player->species, SPECIES_WOBBUFFET); EXPECT_EQ(player->item, ITEM_EJECT_PACK); }
}

SINGLE_BATTLE_TEST("Polished deferred opponent switch items consume before the user's Suction Cups blocks them")
{
    u32 item;
    PARAMETRIZE { item = ITEM_EJECT_BUTTON; }
    PARAMETRIZE { item = ITEM_EJECT_PACK; }
    GIVEN {
        PLAYER(SPECIES_OCTILLERY) { Ability(ABILITY_SUCTION_CUPS); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(item); MaxHP(1600); HP(1600); Speed(50); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, item == ITEM_EJECT_BUTTON ? MOVE_TACKLE : MOVE_GROWL); if (!B_POLISHED_MOVE_BALANCE) SEND_OUT(opponent, 1); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
            ABILITY_POPUP(player, ABILITY_SUCTION_CUPS);
            MESSAGE("Octillery is anchored in place with its suction cups!");
        }
    } THEN {
        EXPECT_EQ(opponent->species, B_POLISHED_MOVE_BALANCE ? SPECIES_WOBBUFFET : SPECIES_CHANSEY);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM), ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished deferred the user's own Eject Pack is not blocked by Suction Cups")
{
    GIVEN {
        PLAYER(SPECIES_OCTILLERY) { Ability(ABILITY_SUCTION_CUPS); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_CLOSE_COMBAT); SEND_OUT(player, 1); } }
    THEN { EXPECT_EQ(player->species, SPECIES_CHANSEY); }
}

SINGLE_BATTLE_TEST("Polished deferred Eject Pack waits for the faint checkpoint")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); Speed(50); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_CLOSE_COMBAT); SEND_OUT(player, 1); SEND_OUT(opponent, 1); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            MESSAGE("The opposing Wobbuffet fainted!");
            MESSAGE("Wobbuffet is switched out with the Eject Pack!");
        }
    }
}

SINGLE_BATTLE_TEST("Polished deferred Eject Pack stays held after the final opponent faints")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_CLOSE_COMBAT); if (!B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); } }
    THEN { if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(player->item, ITEM_EJECT_PACK); }
}

SINGLE_BATTLE_TEST("Polished deferred Baton Pass preserves boosts and does not invite Pursuit")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MaxHP(400); HP(400); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_BATON_PASS); SEND_OUT(player, 1); MOVE(opponent, MOVE_PURSUIT); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 400);
        EXPECT_LT(player->hp, 400);
    }
}

SINGLE_BATTLE_TEST("Polished deferred Baton Pass and trainer Teleport fail without a reserve")
{
    u32 move;
    PARAMETRIZE { move = MOVE_BATON_PASS; }
    PARAMETRIZE { move = MOVE_TELEPORT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(player, move); } }
    SCENE { MESSAGE("But it failed!"); }
}

SINGLE_BATTLE_TEST("Polished deferred trainer Teleport clears boosts without Pursuit interception")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MaxHP(400); HP(400); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SWORDS_DANCE); }
        TURN { MOVE(player, MOVE_TELEPORT); MOVE(opponent, MOVE_PURSUIT); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(player->hp, 400);
    }
}

SINGLE_BATTLE_TEST("Polished deferred Roar reserves before the user's negative-stage Eject Pack")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_ONIX) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_ONIX) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_BATON_PASS); SEND_OUT(player, 1); }
        TURN { MOVE(player, MOVE_ROAR); }
    } THEN {
        EXPECT_EQ(player->species, SPECIES_CHANSEY);
        EXPECT_EQ(player->item, ITEM_EJECT_PACK);
        EXPECT_EQ(opponent->species, SPECIES_ONIX);
    }
}

SINGLE_BATTLE_TEST("Polished deferred trainer Teleport is independent of the native generation preset")
{
    GIVEN {
        WITH_CONFIG(B_TELEPORT_BEHAVIOR, GEN_7);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN { TURN { MOVE(player, MOVE_TELEPORT); if (B_POLISHED_MOVE_BALANCE) SEND_OUT(player, 1); } }
    THEN { EXPECT_EQ(player->species, B_POLISHED_MOVE_BALANCE ? SPECIES_CHANSEY : SPECIES_WOBBUFFET); }
}

SINGLE_BATTLE_TEST("Polished deferred Red Card can trigger again on a later action")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Moves(MOVE_TACKLE); MaxHP(1600); HP(1600); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); Moves(MOVE_TACKLE); MaxHP(1600); HP(1600); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); Speed(50); MaxHP(1600); HP(1600); }
        OPPONENT(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); Speed(50); MaxHP(1600); HP(1600); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
        TURN { SWITCH(opponent, 1); MOVE(player, MOVE_TACKLE); }
    } THEN { EXPECT_EQ(player->species, SPECIES_WOBBUFFET); EXPECT_EQ(opponent->item, ITEM_NONE); }
}

SINGLE_BATTLE_TEST("Polished deferred Mirror Herb resolves before the reserved Eject Pack switch")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_MIRROR_HERB); Speed(50); }
    } WHEN { TURN { MOVE(player, MOVE_SHELL_SMASH); SEND_OUT(player, 1); } }
    SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
            MESSAGE("Wobbuffet is switched out with the Eject Pack!");
        } else {
            MESSAGE("Wobbuffet is switched out with the Eject Pack!");
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, opponent);
        }
    } THEN { EXPECT_EQ(opponent->item, ITEM_NONE); }
}

SINGLE_BATTLE_TEST("Polished deferred optional Overheat retains native Eject Pack timing")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_EJECT_PACK); Speed(100); }
        PLAYER(SPECIES_CHANSEY) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1600); HP(1600); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_NASTY_PLOT); }
        TURN { MOVE(player, MOVE_OVERHEAT); SEND_OUT(player, 1); }
    } THEN { EXPECT_EQ(player->species, SPECIES_CHANSEY); }
}
