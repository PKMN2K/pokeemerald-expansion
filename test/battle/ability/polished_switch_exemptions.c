#include "global.h"
#include "test/battle.h"

ASSUMPTIONS
{
    ASSUME(SHOULD_SWITCH_PERISH_SONG_PERCENTAGE == 100);
    ASSUME(GetItemHoldEffect(ITEM_SHED_SHELL) == HOLD_EFFECT_SHED_SHELL);
}

AI_SINGLE_BATTLE_TEST("Polished switch exemptions let Shed Shell holders escape source traps")
{
    u32 kind = 0;
    enum Item item = ITEM_SHED_SHELL;
    for (u32 trap = 0; trap < 6; trap++)
        for (u32 held = 0; held < 3; held++)
            PARAMETRIZE { kind = trap; item = held == 0 ? ITEM_SHED_SHELL : held == 1 ? ITEM_SMOKE_BALL : ITEM_NONE; }
    enum Ability ability = kind == 3 ? ABILITY_ARENA_TRAP : kind == 4 ? ABILITY_MAGNET_PULL : kind == 5 ? ABILITY_SHADOW_TAG : ABILITY_KEEN_EYE;
    bool32 switches = kind == 0 || (B_POLISHED_MOVE_BALANCE && item == ITEM_SHED_SHELL);
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Item(ITEM_GRIP_CLAW); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(SPECIES_MAGNEMITE) { Ability(ABILITY_KEEN_EYE); Item(item); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, kind == 1 ? MOVE_MEAN_LOOK : kind == 2 ? MOVE_WRAP : MOVE_CELEBRATE, hit: TRUE); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN {
            SWITCH(player, 1);
            if (switches) EXPECT_SWITCH(opponent, 1);
            else { EXPECT_MOVE(opponent, MOVE_SCRATCH); EXPECT_SEND_OUT(opponent, 1); }
        }
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_CROBAT);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM), item);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP) != 0, switches);
    }
}

AI_SINGLE_BATTLE_TEST("Polished switch exemptions let Ghost types leave binding moves")
{
    enum Species species = SPECIES_GASTLY;
    enum Move trap = MOVE_FIRE_SPIN;
    for (u32 mon = 0; mon < 2; mon++)
        for (u32 move = 0; move < 2; move++)
            PARAMETRIZE { species = mon ? SPECIES_MISDREAVUS : SPECIES_GASTLY; trap = move ? MOVE_WHIRLPOOL : MOVE_FIRE_SPIN; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(ITEM_GRIP_CLAW); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(species) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, trap, hit: TRUE); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN {
            SWITCH(player, 1);
            if (B_POLISHED_MOVE_BALANCE) EXPECT_SWITCH(opponent, 1);
            else { EXPECT_MOVE(opponent, MOVE_SCRATCH); EXPECT_SEND_OUT(opponent, 1); }
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP) != 0, B_POLISHED_MOVE_BALANCE);
    }
}

AI_SINGLE_BATTLE_TEST("Polished switch exemptions use the current Shed Shell after Trick")
{
    bool32 gains = FALSE;
    PARAMETRIZE { gains = FALSE; }
    PARAMETRIZE { gains = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(gains ? ITEM_SHED_SHELL : ITEM_NONE); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(gains ? ITEM_NONE : ITEM_SHED_SHELL); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_MEAN_LOOK); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_TRICK); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN {
            SWITCH(player, 1);
            if (B_POLISHED_MOVE_BALANCE && gains) EXPECT_SWITCH(opponent, 1);
            else { EXPECT_MOVE(opponent, MOVE_SCRATCH); EXPECT_SEND_OUT(opponent, 1); }
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM), gains ? ITEM_SHED_SHELL : ITEM_NONE);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP) != 0, B_POLISHED_MOVE_BALANCE && gains);
    }
}

AI_SINGLE_BATTLE_TEST("Polished switch exemptions recompute Ghost eligibility after a live type change")
{
    bool32 gains = FALSE;
    PARAMETRIZE { gains = FALSE; }
    PARAMETRIZE { gains = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(ITEM_GRIP_CLAW); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(gains ? SPECIES_WOBBUFFET : SPECIES_GASTLY) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, gains ? MOVE_MEAN_LOOK : MOVE_FIRE_SPIN, hit: TRUE); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, gains ? MOVE_TRICK_OR_TREAT : MOVE_SOAK); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN {
            SWITCH(player, 1);
            if (B_POLISHED_MOVE_BALANCE && gains) EXPECT_SWITCH(opponent, 1);
            else { EXPECT_MOVE(opponent, MOVE_SCRATCH); EXPECT_SEND_OUT(opponent, 1); }
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP) != 0, B_POLISHED_MOVE_BALANCE && gains);
    }
}

AI_SINGLE_BATTLE_TEST("Polished switch exemptions keep Shed Shell active under Unnerve and Neutralizing Gas")
{
    enum Ability ability = ABILITY_UNNERVE;
    PARAMETRIZE { ability = ABILITY_UNNERVE; }
    PARAMETRIZE { ability = ABILITY_NEUTRALIZING_GAS; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(ITEM_SHED_SHELL); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_MEAN_LOOK); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN {
            SWITCH(player, 1);
            if (B_POLISHED_MOVE_BALANCE) EXPECT_SWITCH(opponent, 1);
            else { EXPECT_MOVE(opponent, MOVE_SCRATCH); EXPECT_SEND_OUT(opponent, 1); }
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP) != 0, B_POLISHED_MOVE_BALANCE);
    }
}

AI_SINGLE_BATTLE_TEST("Polished switch exemptions respect optional item suppression and its removal")
{
    u32 reason = 0;
    for (u32 kind = 0; kind < 3; kind++) PARAMETRIZE { reason = kind; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(reason == 2 ? ABILITY_NEUTRALIZING_GAS : ABILITY_KEEN_EYE); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(reason == 1 ? ABILITY_KEEN_EYE : ABILITY_KLUTZ); Item(ITEM_SHED_SHELL); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_MEAN_LOOK); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, reason == 1 ? MOVE_EMBARGO : MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN {
            SWITCH(player, 1);
            if (B_POLISHED_MOVE_BALANCE && reason == 2) EXPECT_SWITCH(opponent, 1);
            else { EXPECT_MOVE(opponent, MOVE_SCRATCH); EXPECT_SEND_OUT(opponent, 1); }
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP) != 0, B_POLISHED_MOVE_BALANCE && reason == 2);
    }
}

AI_SINGLE_BATTLE_TEST("Polished switch exemptions retain native eligibility with an active gimmick")
{
    bool32 ghost = FALSE;
    PARAMETRIZE { ghost = FALSE; }
    PARAMETRIZE { ghost = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(ITEM_GRIP_CLAW); TeraType(TYPE_PSYCHIC); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(ghost ? SPECIES_GASTLY : SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(ghost ? ITEM_NONE : ITEM_SHED_SHELL); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_FIRE_SPIN, hit: TRUE, gimmick: GIMMICK_TERA); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { SWITCH(player, 1); EXPECT_MOVE(opponent, MOVE_SCRATCH); EXPECT_SEND_OUT(opponent, 1); }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 0);
    }
}

AI_SINGLE_BATTLE_TEST("Polished switch exemptions retain optional Fairy Lock behavior")
{
    bool32 ghost = FALSE, lock = FALSE;
    for (u32 type = 0; type < 2; type++)
        for (u32 field = 0; field < 2; field++)
            PARAMETRIZE { ghost = type; lock = field; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(ITEM_GRIP_CLAW); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(ghost ? SPECIES_GASTLY : SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(ghost ? ITEM_NONE : ITEM_SHED_SHELL); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_FIRE_SPIN, hit: TRUE); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, lock ? MOVE_FAIRY_LOCK : MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN {
            SWITCH(player, 1);
            if (B_POLISHED_MOVE_BALANCE && !lock) EXPECT_SWITCH(opponent, 1);
            else { EXPECT_MOVE(opponent, MOVE_SCRATCH); EXPECT_SEND_OUT(opponent, 1); }
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP) != 0, B_POLISHED_MOVE_BALANCE && !lock);
    }
}

AI_SINGLE_BATTLE_TEST("Polished switch exemptions still require a living eligible reserve")
{
    bool32 fainted = FALSE;
    PARAMETRIZE { fainted = FALSE; }
    PARAMETRIZE { fainted = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); Item(ITEM_SHED_SHELL); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        if (fainted) OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(0); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_MEAN_LOOK); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { SWITCH(player, 1); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 0);
    }
}
