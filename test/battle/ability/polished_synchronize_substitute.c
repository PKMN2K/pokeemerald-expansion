#include "global.h"
#include "test/battle.h"

static const enum Move sStatusMoves[] = {MOVE_POISONPOWDER, MOVE_STUN_SPORE, MOVE_THUNDER_WAVE, MOVE_TOXIC, MOVE_GLARE, MOVE_WILL_O_WISP};

static u32 SyncStatus(enum Move move)
{
    switch (move)
    {
    case MOVE_POISONPOWDER:
    case MOVE_POISON_GAS:
    case MOVE_POISON_STING:
        return STATUS1_POISON;
    case MOVE_TOXIC:
        return STATUS1_TOXIC_POISON;
    case MOVE_WILL_O_WISP:
    case MOVE_EMBER:
        return STATUS1_BURN;
    default:
        return STATUS1_PARALYSIS;
    }
}

static void ExpectSyncStatus(struct BattlePokemon *battler, u32 status)
{
    EXPECT_EQ(battler->status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON | STATUS1_BURN | STATUS1_PARALYSIS), status);
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute protects the original attacker from direct status returns")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    bool32 sub = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sStatusMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 shield = 0; shield < 2; shield++)
                PARAMETRIZE { move = sStatusMoves[m]; user = side ? opponent : player; target = side ? player : opponent; sub = shield; }
    u32 status = SyncStatus(move);
    bool32 blocked = B_POLISHED_MOVE_BALANCE && sub;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); MovesWithPP({move, 2}, {MOVE_SUBSTITUTE, 2}, {MOVE_CELEBRATE, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); MovesWithPP({move, 2}, {MOVE_SUBSTITUTE, 2}, {MOVE_CELEBRATE, 10}); }
    } WHEN {
        if (sub) TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        STATUS_ICON(target, status);
        ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
        if (blocked) NOT STATUS_ICON(user, status);
        else STATUS_ICON(user, status);
    } THEN {
        ExpectSyncStatus(user, blocked ? STATUS1_NONE : status);
        ExpectSyncStatus(target, status);
        EXPECT_EQ(user->pp[0], 1);
        EXPECT_EQ((u32)user->volatiles.substituteHP, sub ? 100 : 0);
        if (blocked) EXPECT_EQ(user->hp, 300);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute uses the reflector ability instead of the original attacker ability")
{
    static const enum Ability abilities[] = {ABILITY_INFILTRATOR, ABILITY_CORROSION, ABILITY_MOLD_BREAKER};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_INFILTRATOR;
    for (u32 m = 0; m < ARRAY_COUNT(sStatusMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
                PARAMETRIZE { move = sStatusMoves[m]; user = side ? opponent : player; target = side ? player : opponent; ability = abilities[a]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ability : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ability : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
    } THEN {
        ExpectSyncStatus(user, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : SyncStatus(move));
        ExpectSyncStatus(target, SyncStatus(move));
        EXPECT_EQ((u32)user->volatiles.substituteHP, 100);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute distinguishes outgoing Infiltrator from the returned status")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 m = 0; m < ARRAY_COUNT(sStatusMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sStatusMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ABILITY_INFILTRATOR : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_INFILTRATOR : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN {
        ExpectSyncStatus(user, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : SyncStatus(move));
        ExpectSyncStatus(target, SyncStatus(move));
        EXPECT_EQ((u32)user->volatiles.substituteHP, 100);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 100);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute preserves the source outgoing Corrosion exception")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 m = 0; m < 2; m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = m ? MOVE_TOXIC : MOVE_POISONPOWDER; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ABILITY_CORROSION : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
        else NOT ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
    } THEN {
        ExpectSyncStatus(user, STATUS1_NONE);
        ExpectSyncStatus(target, B_POLISHED_MOVE_BALANCE ? SyncStatus(move) : STATUS1_NONE);
        EXPECT_EQ((u32)user->volatiles.substituteHP, 100);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 100);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute preserves the protected attacker healing berry")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    bool32 sub = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sStatusMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 shield = 0; shield < 2; shield++)
                PARAMETRIZE { move = sStatusMoves[m]; user = side ? opponent : player; target = side ? player : opponent; sub = shield; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); Item(user == player ? ITEM_LUM_BERRY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); Item(user == opponent ? ITEM_LUM_BERRY : ITEM_NONE); }
    } WHEN {
        if (sub) TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN {
        ExpectSyncStatus(user, STATUS1_NONE);
        ExpectSyncStatus(target, SyncStatus(move));
        EXPECT_EQ(user->item, B_POLISHED_MOVE_BALANCE && sub ? ITEM_LUM_BERRY : ITEM_NONE);
        EXPECT_EQ(user->hp, sub ? 300 : 400);
        EXPECT_EQ((u32)user->volatiles.substituteHP, sub ? 100 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute still checks the return before the reflector berry heals")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 m = 0; m < ARRAY_COUNT(sStatusMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sStatusMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); Item(target == player ? ITEM_LUM_BERRY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); Item(target == opponent ? ITEM_LUM_BERRY : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        STATUS_ICON(target, SyncStatus(move));
        ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, target);
        STATUS_ICON(target, none: TRUE);
    } THEN {
        ExpectSyncStatus(user, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : SyncStatus(move));
        ExpectSyncStatus(target, STATUS1_NONE);
        EXPECT_EQ(target->item, ITEM_NONE);
        EXPECT_EQ(target->hp, 400);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute respects suppression of Synchronize")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 m = 0; m < ARRAY_COUNT(sStatusMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sStatusMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_GASTRO_ACID); }
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        NOT ABILITY_POPUP(target, ABILITY_SYNCHRONIZE);
    } THEN {
        ExpectSyncStatus(user, STATUS1_NONE);
        ExpectSyncStatus(target, SyncStatus(move));
        EXPECT_EQ((u32)user->volatiles.substituteHP, 100);
    }
}

DOUBLE_BATTLE_TEST("Polished Synchronize Substitute leaves doubles native")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 m = 0; m < ARRAY_COUNT(sStatusMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sStatusMoves[m]; user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == playerLeft ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponentLeft ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, target: target, hit: TRUE); }
    } THEN {
        ExpectSyncStatus(user, SyncStatus(move));
        ExpectSyncStatus(target, SyncStatus(move));
        EXPECT_EQ((u32)user->volatiles.substituteHP, 100);
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute leaves either active Terastal native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    bool32 userTera = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sStatusMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 tera = 0; tera < 2; tera++)
                PARAMETRIZE { move = sStatusMoves[m]; user = side ? opponent : player; target = side ? player : opponent; userTera = tera; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(target, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } THEN {
        ExpectSyncStatus(user, SyncStatus(move));
        ExpectSyncStatus(target, SyncStatus(move));
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute leaves optional status moves and damaging effects native")
{
    static const enum Move moves[] = {MOVE_POISON_GAS, MOVE_POISON_STING, MOVE_EMBER, MOVE_THUNDER_SHOCK};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_GAS;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = moves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_NONE : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } THEN {
        ExpectSyncStatus(user, SyncStatus(move));
        ExpectSyncStatus(target, SyncStatus(move));
    }
}

SINGLE_BATTLE_TEST("Polished Synchronize Substitute leaves dynamically retyped status moves native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 m = 0; m < ARRAY_COUNT(sStatusMoves); m++)
        if (sStatusMoves[m] != MOVE_GLARE)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { move = sStatusMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(400); HP(400); Ability(user == player ? ABILITY_NORMALIZE : ABILITY_SYNCHRONIZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(400); HP(400); Ability(user == opponent ? ABILITY_NORMALIZE : ABILITY_SYNCHRONIZE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN {
        ExpectSyncStatus(user, SyncStatus(move));
        ExpectSyncStatus(target, SyncStatus(move));
    }
}
