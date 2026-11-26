#include "global.h"
#include "test/battle.h"
#include "battle_util.h"

SINGLE_BATTLE_TEST("Polished Choice history records selection before flinching or move failure")
{
    enum Move move = MOVE_SPLASH;
    enum Move interrupt = MOVE_SPLASH;
    PARAMETRIZE { move = MOVE_SPLASH; interrupt = MOVE_SPLASH; }
    PARAMETRIZE { move = MOVE_GROWL; interrupt = MOVE_FAKE_OUT; }
    PARAMETRIZE { move = MOVE_REST; interrupt = MOVE_SPLASH; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); Moves(move, MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
    } WHEN {
        TURN { MOVE(player, move); MOVE(opponent, interrupt); }
    } THEN {
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? move : MOVE_NONE);
        // History by itself imposes no restriction until a Choice effect exists.
        EXPECT_EQ(CheckMoveLimitations(B_BATTLER_0, 0, MOVE_LIMITATION_CHOICE_ITEM), 0);
    }
}

SINGLE_BATTLE_TEST("Polished Choice history is updated by each free selection in every slot")
{
    u32 slot = 0;
    for (u32 i = 0; i < 4; i++) PARAMETRIZE { slot = i; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(slot == 0 ? MOVE_GROWL : MOVE_SPLASH, slot == 1 ? MOVE_GROWL : MOVE_SPLASH,
                                        slot == 2 ? MOVE_GROWL : MOVE_SPLASH, slot == 3 ? MOVE_GROWL : MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_GROWL); }
    } THEN {
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? MOVE_GROWL : MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Choice Trick retains the selected Trick when acquiring a Choice item")
{
    enum Item item = ITEM_CHOICE_BAND;
    PARAMETRIZE { item = ITEM_CHOICE_BAND; }
    PARAMETRIZE { item = ITEM_CHOICE_SPECS; }
    PARAMETRIZE { item = ITEM_CHOICE_SCARF; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_TRICK, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Item(item); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRICK); }
    } THEN {
        EXPECT_EQ(player->item, item);
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? MOVE_TRICK : MOVE_NONE);
        EXPECT_EQ(CheckMoveLimitations(B_BATTLER_0, 0, MOVE_LIMITATION_CHOICE_ITEM) & 3, B_POLISHED_MOVE_BALANCE ? 2 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Choice history survives item removal and changes on the next free selection")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Item(ITEM_CHOICE_BAND); Moves(MOVE_GROWL, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL); MOVE(opponent, MOVE_KNOCK_OFF); }
        TURN { MOVE(player, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? MOVE_SPLASH : MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Choice Encore changes history even when Mental Herb immediately cures it")
{
    bool32 actsFirst = FALSE;
    PARAMETRIZE { actsFirst = FALSE; }
    PARAMETRIZE { actsFirst = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(actsFirst ? 100 : 50); Item(ITEM_MENTAL_HERB); Moves(MOVE_GROWL, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(actsFirst ? 50 : 100); }
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_ENCORE); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ((u32)player->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? (actsFirst ? MOVE_SPLASH : MOVE_GROWL) : MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Choice Encore history remains after its duration expires")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); Moves(MOVE_GROWL, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_GROWL); MOVE(opponent, MOVE_ENCORE); }
        TURN { MOVE(player, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_GROWL); }
        TURN { MOVE(player, MOVE_GROWL); }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? MOVE_GROWL : MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Choice switching resets the incoming battlers history")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_CHOICE_BAND); Moves(MOVE_GROWL, MOVE_SPLASH); }
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_CHOICE_BAND); Moves(MOVE_GROWL, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL); }
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), MOVE_NONE);
        EXPECT_EQ(CheckMoveLimitations(B_BATTLER_0, 0, MOVE_LIMITATION_CHOICE_ITEM) & 3, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Choice Transform clears history when the selected move is absent in copied moves")
{
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_NONE); Moves(MOVE_TRANSFORM, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE, MOVE_GROWL); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, MOVE_GROWL); }
    } THEN {
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Choice Transform preserves history if the copied moves still include Transform")
{
    GIVEN {
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_NONE); Moves(MOVE_TRANSFORM, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE, MOVE_TRANSFORM, MOVE_GROWL); }
    } WHEN {
        TURN { MOVE(player, MOVE_TRANSFORM); MOVE(opponent, MOVE_GROWL); }
    } THEN {
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? MOVE_TRANSFORM : MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Choice called moves retain the selected caller history")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_METRONOME, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_METRONOME, WITH_RNG(RNG_METRONOME, MOVE_GROWL)); }
    } THEN {
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? MOVE_METRONOME : MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Choice Gorilla Tactics gained after acting restricts the previously selected move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_NONE); Moves(MOVE_GROWL, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_GORILLA_TACTICS); }
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL); MOVE(opponent, MOVE_SKILL_SWAP); }
    } THEN {
        EXPECT_EQ(player->ability, ABILITY_GORILLA_TACTICS);
        EXPECT_EQ(GetChoiceLockedMove(B_BATTLER_0), B_POLISHED_MOVE_BALANCE ? MOVE_GROWL : MOVE_NONE);
        EXPECT_EQ(CheckMoveLimitations(B_BATTLER_0, 0, MOVE_LIMITATION_CHOICE_ITEM) & 3, B_POLISHED_MOVE_BALANCE ? 2 : 0);
    }
}
