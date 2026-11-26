#include "global.h"
#include "test/battle.h"

static const enum Move sPowderMoves[] = {MOVE_POISONPOWDER, MOVE_SLEEP_POWDER, MOVE_SPORE, MOVE_STUN_SPORE};

enum PowderBarrier { POWDER_NONE, POWDER_GRASS, POWDER_GOGGLES, POWDER_OVERCOAT };

static void ExpectPowderStatus(struct BattlePokemon *battler, enum Move move, bool32 affected)
{
    if (move == MOVE_POISONPOWDER) EXPECT_EQ(!!(battler->status1 & STATUS1_POISON), !!affected);
    else if (move == MOVE_STUN_SPORE) EXPECT_EQ(!!(battler->status1 & STATUS1_PARALYSIS), !!affected);
    else EXPECT_EQ(!!(battler->status1 & STATUS1_SLEEP), !!affected);
}

SINGLE_BATTLE_TEST("Polished Corrosion powder bypass applies only to PoisonPowder")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    u32 barrier = POWDER_NONE;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPowderMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 kind = POWDER_NONE; kind <= POWDER_OVERCOAT; kind++)
                PARAMETRIZE { move = sPowderMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; barrier = kind; }
    bool32 affected = barrier == POWDER_NONE || (B_POLISHED_MOVE_BALANCE && move == MOVE_POISONPOWDER);
    GIVEN {
        PLAYER(target == player && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_CORROSION : barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_NONE); Item(target == player && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); MovesWithPP({move, 2}); }
        OPPONENT(target == opponent && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_CORROSION : barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_NONE); Item(target == opponent && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); MovesWithPP({move, 2}); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        if (affected) ANIMATION(ANIM_TYPE_MOVE, move, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        ExpectPowderStatus(target, move, affected);
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(user->pp[0], 1);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion powder distinguishes ordinary users and Mold Breaker")
{
    static const enum Ability abilities[] = {ABILITY_NONE, ABILITY_MOLD_BREAKER, ABILITY_CORROSION};
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    u32 barrier = POWDER_GRASS;
    for (u32 entry = 0; entry < ARRAY_COUNT(abilities); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 kind = POWDER_GRASS; kind <= POWDER_OVERCOAT; kind++)
                PARAMETRIZE { ability = abilities[entry]; user = side ? opponent : player; target = side ? player : opponent; barrier = kind; }
    bool32 affected = (ability == ABILITY_MOLD_BREAKER && barrier == POWDER_OVERCOAT) || (B_POLISHED_MOVE_BALANCE && ability == ABILITY_CORROSION);
    GIVEN {
        PLAYER(target == player && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ability : barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_NONE); Item(target == player && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
        OPPONENT(target == opponent && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ability : barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_NONE); Item(target == opponent && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_POISONPOWDER, hit: TRUE); }
    } THEN {
        ExpectPowderStatus(target, MOVE_POISONPOWDER, affected);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion powder retains Protect Safeguard existing status and accuracy")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 gate = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 4; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; gate = kind; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); if (target == player && gate == 2) Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); if (target == opponent && gate == 2) Status1(STATUS1_BURN); }
    } WHEN {
        if (gate == 1) TURN { MOVE(target, MOVE_SAFEGUARD); }
        TURN { if (gate == 0) MOVE(target, MOVE_PROTECT); MOVE(user, MOVE_POISONPOWDER, hit: gate != 3); }
    } THEN {
        EXPECT_EQ(target->status1, gate == 2 ? STATUS1_BURN : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion powder preserves Magic Bounce on an immune original target")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    u32 barrier = POWDER_GRASS;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPowderMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 kind = POWDER_GRASS; kind <= POWDER_GOGGLES; kind++)
                PARAMETRIZE { move = sPowderMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; barrier = kind; }
    GIVEN {
        PLAYER(target == player && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_CORROSION : ABILITY_MAGIC_BOUNCE); Item(target == player && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
        OPPONENT(target == opponent && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_MAGIC_BOUNCE); Item(target == opponent && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        // Source checkpowder runs before bounceback, and ResetTurn retains its
        // immunity result. Only Corrosion PoisonPowder skipped the original
        // Grass/Goggles check; the other powders bounce but remain ineffective.
        ExpectPowderStatus(user, move, !B_POLISHED_MOVE_BALANCE || move == MOVE_POISONPOWDER);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion powder returned moves use the actual reflector ability")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISONPOWDER;
    u32 barrier = POWDER_GRASS;
    for (u32 entry = 0; entry < ARRAY_COUNT(sPowderMoves); entry++)
        for (u32 side = 0; side < 2; side++)
            for (u32 kind = POWDER_GRASS; kind <= POWDER_OVERCOAT; kind++)
                PARAMETRIZE { move = sPowderMoves[entry]; user = side ? opponent : player; target = side ? player : opponent; barrier = kind; }
    GIVEN {
        PLAYER(user == player && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(100); Ability(target == player ? ABILITY_MAGIC_BOUNCE : barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_CORROSION); Item(user == player && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
        OPPONENT(user == opponent && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(50); Ability(target == opponent ? ABILITY_MAGIC_BOUNCE : barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_CORROSION); Item(user == opponent && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion powder leaves optional Magic Coat native")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 barrier = POWDER_GRASS;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = POWDER_GRASS; kind <= POWDER_GOGGLES; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; barrier = kind; }
    GIVEN {
        PLAYER(user == player && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(100); Ability(target == player ? ABILITY_CORROSION : ABILITY_NONE); Item(user == player && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
        OPPONENT(user == opponent && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(50); Ability(target == opponent ? ABILITY_CORROSION : ABILITY_NONE); Item(user == opponent && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_MAGIC_COAT); MOVE(user, MOVE_POISONPOWDER, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MAGIC_COAT, target);
    } THEN {
        EXPECT_EQ(user->status1, STATUS1_NONE);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion powder leaves either active Terastal native")
{
    u32 barrier = POWDER_GRASS;
    bool32 userTera = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = POWDER_GRASS; kind <= POWDER_OVERCOAT; kind++)
            PARAMETRIZE { userTera = side; barrier = kind; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_CORROSION); TeraType(TYPE_NORMAL); }
        OPPONENT(barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(50); Ability(barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_NONE); Item(barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); TeraType(barrier == POWDER_GRASS ? TYPE_GRASS : TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(player, MOVE_POISONPOWDER, hit: TRUE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(opponent, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } THEN {
        EXPECT_EQ(opponent->status1, STATUS1_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished Corrosion powder leaves doubles native")
{
    u32 barrier = POWDER_GRASS;
    for (u32 kind = POWDER_GRASS; kind <= POWDER_OVERCOAT; kind++)
        PARAMETRIZE { barrier = kind; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_CORROSION); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(50); Ability(barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_NONE); Item(barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_POISONPOWDER, target: opponentLeft, hit: TRUE); }
    } THEN {
        EXPECT_EQ(opponentLeft->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion powder leaves dynamically retyped moves native")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 barrier = POWDER_GRASS;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = POWDER_GRASS; kind <= POWDER_GOGGLES; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; barrier = kind; }
    GIVEN {
        PLAYER(target == player && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(target == player ? 100 : 50); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); Item(target == player && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
        OPPONENT(target == opponent && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(target == opponent ? 100 : 50); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); Item(target == opponent && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_ELECTRIFY); MOVE(user, MOVE_POISONPOWDER, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ELECTRIFY, target);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_POISONPOWDER, user);
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished Corrosion powder respects suppression of the actual attacker ability")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 barrier = POWDER_GRASS;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = POWDER_GRASS; kind <= POWDER_OVERCOAT; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; barrier = kind; }
    GIVEN {
        PLAYER(target == player && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(100); Ability(user == player ? ABILITY_CORROSION : barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_NONE); Item(target == player && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
        OPPONENT(target == opponent && barrier == POWDER_GRASS ? SPECIES_TANGELA : SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent ? ABILITY_CORROSION : barrier == POWDER_OVERCOAT ? ABILITY_OVERCOAT : ABILITY_NONE); Item(target == opponent && barrier == POWDER_GOGGLES ? ITEM_SAFETY_GOGGLES : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_GASTRO_ACID); }
        TURN { MOVE(user, MOVE_POISONPOWDER, hit: TRUE); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}
