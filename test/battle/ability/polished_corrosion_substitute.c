#include "global.h"
#include "test/battle.h"

static const enum Move sPoisonStatusMoves[] = {MOVE_POISONPOWDER, MOVE_TOXIC};

static void ExpectPoison(struct BattlePokemon *battler, enum Move move, bool32 affected)
{
    u32 mask = STATUS1_POISON | STATUS1_TOXIC_POISON;
    EXPECT_EQ(battler->status1 & mask, affected ? (move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON) : 0);
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute bypass distinguishes ordinary Infiltrator and Mold Breaker users")
{
    static const enum Ability abilities[] = {ABILITY_NONE, ABILITY_CORROSION, ABILITY_INFILTRATOR, ABILITY_MOLD_BREAKER};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Ability ability = ABILITY_NONE;
    bool32 sub = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPoisonStatusMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 kind = 0; kind < ARRAY_COUNT(abilities); kind++)
                for (u32 shield = 0; shield < 2; shield++)
                    PARAMETRIZE { move = sPoisonStatusMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; ability = abilities[kind]; sub = shield; }
    bool32 affected = !sub || ability == ABILITY_INFILTRATOR || (B_POLISHED_MOVE_BALANCE && ability == ABILITY_CORROSION);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(200); HP(200); Ability(user == player ? ability : ABILITY_NONE); MovesWithPP({move, 2}, {MOVE_SUBSTITUTE, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(user == opponent ? ability : ABILITY_NONE); MovesWithPP({move, 2}, {MOVE_SUBSTITUTE, 10}); }
    } WHEN {
        if (sub) TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN {
        ExpectPoison(target, move, affected);
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(user->pp[0], 1);
        if (sub) EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute bypass retains Poison and Steel type coverage")
{
    static const enum Species species[] = {SPECIES_WOBBUFFET, SPECIES_ARBOK, SPECIES_STEELIX};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    enum Species victim = SPECIES_WOBBUFFET;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPoisonStatusMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 kind = 0; kind < ARRAY_COUNT(species); kind++)
                PARAMETRIZE { move = sPoisonStatusMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; victim = species[kind]; }
    GIVEN {
        PLAYER(target == player ? victim : SPECIES_WOBBUFFET) { Speed(100); MaxHP(200); HP(200); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(target == opponent ? victim : SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN {
        ExpectPoison(target, move, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute bypass retains Protect Safeguard existing status and misses")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    u32 gate = 0;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPoisonStatusMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 kind = 0; kind < 4; kind++)
                PARAMETRIZE { move = sPoisonStatusMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; gate = kind; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(200); HP(200); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); if (target == player && gate == 2) Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); if (target == opponent && gate == 2) Status1(STATUS1_BURN); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        if (gate == 1) TURN { MOVE(target, MOVE_SAFEGUARD); }
        TURN { if (gate == 0) MOVE(target, MOVE_PROTECT); MOVE(user, move, hit: gate != 3); }
    } THEN {
        EXPECT_EQ(target->status1, gate == 2 ? STATUS1_BURN : STATUS1_NONE);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute bypass does not enable a protected target Magic Bounce")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    bool32 userSub = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPoisonStatusMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 shield = 0; shield < 2; shield++)
                PARAMETRIZE { move = sPoisonStatusMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; userSub = shield; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(200); HP(200); Ability(user == player ? ABILITY_CORROSION : ABILITY_MAGIC_BOUNCE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_MAGIC_BOUNCE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); if (userSub) MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        else ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        ExpectPoison(target, move, B_POLISHED_MOVE_BALANCE);
        ExpectPoison(user, move, !B_POLISHED_MOVE_BALANCE && !userSub);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
        if (userSub) EXPECT_EQ((u32)user->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute returned moves do not inherit the original user ability")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPoisonStatusMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sPoisonStatusMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(200); HP(200); Ability(user == player ? ABILITY_CORROSION : ABILITY_MAGIC_BOUNCE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_MAGIC_BOUNCE); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ((u32)user->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute leaves optional Magic Coat reflection native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPoisonStatusMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sPoisonStatusMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(200); HP(200); Ability(target == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(target == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(target, MOVE_MAGIC_COAT); MOVE(user, move, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MAGIC_COAT, target);
    } THEN {
        ExpectPoison(user, move, TRUE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute leaves either active Terastal native")
{
    enum Move move = MOVE_POISONPOWDER;
    bool32 userTera = FALSE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPoisonStatusMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sPoisonStatusMoves[entry]; userTera = side; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(200); HP(200); Ability(ABILITY_CORROSION); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(player, move, hit: TRUE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); }
    } THEN {
        EXPECT_EQ(opponent->status1, STATUS1_NONE);
        EXPECT_EQ((u32)opponent->volatiles.substituteHP, 50);
    }
}

DOUBLE_BATTLE_TEST("Polished Corrosion Substitute leaves doubles native")
{
    enum Move move = MOVE_POISONPOWDER;
    PARAMETRIZE { move = MOVE_POISONPOWDER; }
    PARAMETRIZE { move = MOVE_TOXIC; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_CORROSION); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_SUBSTITUTE); }
        TURN { MOVE(playerLeft, move, target: opponentLeft, hit: TRUE); }
    } THEN {
        EXPECT_EQ(opponentLeft->status1, STATUS1_NONE);
        EXPECT_EQ((u32)opponentLeft->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute leaves dynamically retyped moves native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPoisonStatusMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sPoisonStatusMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(target == player ? 100 : 50); MaxHP(200); HP(200); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(target == opponent ? 100 : 50); MaxHP(200); HP(200); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(target, MOVE_ELECTRIFY); MOVE(user, move, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ELECTRIFY, target);
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute respects suppression of the actual attacker ability")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPoisonStatusMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sPoisonStatusMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(200); HP(200); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_GASTRO_ACID); }
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion Substitute leaves other status moves native")
{
    static const enum Move moves[] = {MOVE_SLEEP_POWDER, MOVE_SPORE, MOVE_STUN_SPORE, MOVE_POISON_GAS};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_SLEEP_POWDER;
    for (u32 entry = 0; entry < ARRAY_COUNT(moves); entry++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = moves[entry]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(200); HP(200); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(200); HP(200); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 50);
    }
}
