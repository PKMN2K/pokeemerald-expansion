#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Run Away permits ordinary switches through trapping abilities")
{
    struct BattlePokemon *user = player;
    enum Ability trap = ABILITY_ARENA_TRAP;
    bool32 runAway = TRUE, switchOut = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            for (u32 active = 0; active < 2; active++)
                for (u32 switching = 0; switching < 2; switching++)
                    PARAMETRIZE { user = side ? opponent : player; trap = kind == 0 ? ABILITY_ARENA_TRAP : kind == 1 ? ABILITY_MAGNET_PULL : ABILITY_SHADOW_TAG; runAway = active; switchOut = switching; }
    GIVEN {
        PLAYER(SPECIES_MAGNEMITE) { Ability(user == player ? (runAway ? ABILITY_RUN_AWAY : ABILITY_KEEN_EYE) : trap); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_MAGNEMITE) { Ability(user == opponent ? (runAway ? ABILITY_RUN_AWAY : ABILITY_KEEN_EYE) : trap); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN { if (B_POLISHED_MOVE_BALANCE && runAway && switchOut) SWITCH(user, 1); else MOVE(user, MOVE_CELEBRATE); }
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(user->species, B_POLISHED_MOVE_BALANCE && runAway && switchOut ? SPECIES_WYNAUT : SPECIES_MAGNEMITE);
        if (!switchOut)
            EXPECT_EQ(IsAbilityPreventingEscape(battler), B_POLISHED_MOVE_BALANCE && runAway ? 0 : GetOppositeBattler(battler) + 1);
    }
}

SINGLE_BATTLE_TEST("Polished Run Away permits ordinary switches through Mean Look and Wrap")
{
    struct BattlePokemon *user = player, *foe = opponent;
    enum Move trap = MOVE_MEAN_LOOK;
    bool32 runAway = TRUE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            for (u32 active = 0; active < 2; active++)
                PARAMETRIZE { user = side ? opponent : player; foe = side ? player : opponent; trap = kind ? MOVE_WRAP : MOVE_MEAN_LOOK; runAway = active; }
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(user == player && runAway ? ABILITY_RUN_AWAY : ABILITY_KEEN_EYE); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(SPECIES_RATTATA) { Ability(user == opponent && runAway ? ABILITY_RUN_AWAY : ABILITY_KEEN_EYE); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(25); }
    } WHEN {
        TURN { MOVE(foe, trap, hit: TRUE); }
        TURN { if (B_POLISHED_MOVE_BALANCE && runAway) SWITCH(user, 1); else MOVE(user, MOVE_CELEBRATE); }
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(user->species, B_POLISHED_MOVE_BALANCE && runAway ? SPECIES_WYNAUT : SPECIES_RATTATA);
        EXPECT_EQ(CanBattlerEscape(battler), B_POLISHED_MOVE_BALANCE && runAway);
    }
}

SINGLE_BATTLE_TEST("Polished Run Away permits escape without erasing the trap or residual damage")
{
    struct BattlePokemon *user = player, *foe = opponent;
    enum Move trap = MOVE_MEAN_LOOK;
    bool32 runAway = TRUE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            for (u32 active = 0; active < 2; active++)
                PARAMETRIZE { user = side ? opponent : player; foe = side ? player : opponent; trap = kind ? MOVE_WRAP : MOVE_MEAN_LOOK; runAway = active; }
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(user == player && runAway ? ABILITY_RUN_AWAY : ABILITY_KEEN_EYE); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_RATTATA) { Ability(user == opponent && runAway ? ABILITY_RUN_AWAY : ABILITY_KEEN_EYE); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(foe, trap, hit: TRUE); }
    } SCENE {
        if (trap == MOVE_WRAP) { HP_BAR(user); HP_BAR(user); }
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(CanBattlerEscape(battler), B_POLISHED_MOVE_BALANCE && runAway);
        EXPECT_EQ((u32)user->volatiles.escapePrevention, trap == MOVE_MEAN_LOOK);
        EXPECT_EQ((u32)user->volatiles.wrapped, trap == MOVE_WRAP);
    }
}

SINGLE_BATTLE_TEST("Polished Run Away respects Neutralizing Gas and optional Gastro Acid suppression")
{
    struct BattlePokemon *user = player, *foe = opponent;
    bool32 gas = TRUE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; foe = side ? player : opponent; gas = kind; }
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(user == player ? ABILITY_RUN_AWAY : gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RATTATA) { Ability(user == opponent ? ABILITY_RUN_AWAY : gas ? ABILITY_NEUTRALIZING_GAS : ABILITY_KEEN_EYE); }
    } WHEN {
        if (!gas)
            TURN { MOVE(foe, MOVE_GASTRO_ACID); }
        TURN { MOVE(foe, MOVE_MEAN_LOOK); }
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(GetBattlerAbilityIgnoreMoldBreaker(battler), ABILITY_NONE);
        EXPECT_EQ(CanBattlerEscape(battler), FALSE);
        EXPECT_EQ((u32)user->volatiles.escapePrevention, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Run Away retains native switching gates during either side's active Terastal")
{
    struct BattlePokemon *user = player, *foe = opponent;
    bool32 abilityTrap = FALSE, userTera = TRUE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            for (u32 who = 0; who < 2; who++)
                PARAMETRIZE { user = side ? opponent : player; foe = side ? player : opponent; abilityTrap = kind; userTera = who; }
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(user == player ? ABILITY_RUN_AWAY : abilityTrap ? ABILITY_ARENA_TRAP : ABILITY_KEEN_EYE); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_RATTATA) { Ability(user == opponent ? ABILITY_RUN_AWAY : abilityTrap ? ABILITY_ARENA_TRAP : ABILITY_KEEN_EYE); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(user, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(foe, abilityTrap ? MOVE_CELEBRATE : MOVE_MEAN_LOOK, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
        if (abilityTrap)
            EXPECT_EQ(IsAbilityPreventingEscape(battler), GetOppositeBattler(battler) + 1);
        else
            EXPECT_EQ(CanBattlerEscape(battler), FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Run Away retains optional Ingrain and Fairy Lock restrictions")
{
    struct BattlePokemon *user = player;
    enum Move setup = MOVE_INGRAIN;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; setup = kind ? MOVE_FAIRY_LOCK : MOVE_INGRAIN; }
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_RUN_AWAY); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(user, setup); }
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(user == player ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(CanBattlerEscape(battler), FALSE);
    }
}

DOUBLE_BATTLE_TEST("Polished Run Away retains native switching gates in optional doubles")
{
    struct BattlePokemon *user = playerLeft, *foe = opponentLeft;
    bool32 abilityTrap = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponentLeft : playerLeft; foe = side ? playerLeft : opponentLeft; abilityTrap = kind; }
    GIVEN {
        PLAYER(SPECIES_RATTATA) { Ability(user == playerLeft ? ABILITY_RUN_AWAY : abilityTrap ? ABILITY_ARENA_TRAP : ABILITY_KEEN_EYE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RATTATA) { Ability(user == opponentLeft ? ABILITY_RUN_AWAY : abilityTrap ? ABILITY_ARENA_TRAP : ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN { if (!abilityTrap) MOVE(foe, MOVE_MEAN_LOOK, target: user); }
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(user == playerLeft ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
        if (abilityTrap)
            EXPECT_EQ(IsAbilityPreventingEscape(battler), GetOppositeBattler(battler) + 1);
        else
            EXPECT_EQ(CanBattlerEscape(battler), FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("Polished Run Away allows the AI to select a switch despite trapping")
{
    u32 trap = 0;
    for (u32 kind = 0; kind < 4; kind++)
        PARAMETRIZE { trap = kind; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        ASSUME(SHOULD_SWITCH_PERISH_SONG_PERCENTAGE == 100);
        PLAYER(SPECIES_WOBBUFFET) { Ability(trap == 3 ? ABILITY_SHADOW_TAG : ABILITY_KEEN_EYE); Item(ITEM_GRIP_CLAW); Speed(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_RUN_AWAY); Moves(MOVE_SCRATCH); Speed(50); HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_CROBAT) { Ability(ABILITY_KEEN_EYE); Moves(MOVE_SCRATCH); Speed(150); HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, trap == 1 ? MOVE_MEAN_LOOK : trap == 2 ? MOVE_WRAP : MOVE_CELEBRATE, hit: TRUE); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { MOVE(player, MOVE_PERISH_SONG); EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN { EXPECT_MOVE(opponent, MOVE_SCRATCH); }
        TURN {
            SWITCH(player, 1);
            if (B_POLISHED_MOVE_BALANCE || trap == 0)
                EXPECT_SWITCH(opponent, 1);
            else {
                EXPECT_MOVE(opponent, MOVE_SCRATCH);
                EXPECT_SEND_OUT(opponent, 1);
            }
        }
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_CROBAT);
    }
}
