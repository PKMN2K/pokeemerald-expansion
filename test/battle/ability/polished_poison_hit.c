#include "global.h"
#include "test/battle.h"

static const enum Move sPoisonHitMoves[] = {MOVE_POISON_STING, MOVE_GUNK_SHOT, MOVE_POISON_JAB, MOVE_SLUDGE_BOMB};

SINGLE_BATTLE_TEST("Polished poison hit Corrosion cannot poison Poison types with damaging moves")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_STING;
    bool32 corrosion = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonHitMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 ability = 0; ability < 2; ability++)
                PARAMETRIZE { move = sPoisonHitMoves[m]; user = side ? opponent : player; target = side ? player : opponent; corrosion = ability; }
    bool32 poisoned = corrosion && !B_POLISHED_MOVE_BALANCE;
    GIVEN {
        ASSUME(MoveHasAdditionalEffect(move, MOVE_EFFECT_POISON));
        PLAYER(target == player ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Ability(user == player && corrosion ? ABILITY_CORROSION : ABILITY_NONE); MovesWithPP({move, 2}); }
        OPPONENT(target == opponent ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Ability(user == opponent && corrosion ? ABILITY_CORROSION : ABILITY_NONE); MovesWithPP({move, 2}); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        if (poisoned) STATUS_ICON(target, poison: TRUE);
        else NOT STATUS_ICON(target, poison: TRUE);
    } THEN {
        EXPECT_EQ(target->status1, poisoned ? STATUS1_POISON : STATUS1_NONE);
        EXPECT_EQ(user->pp[0], 1);
        EXPECT_LT(target->hp, 500);
    }
}

SINGLE_BATTLE_TEST("Polished poison hit still poisons susceptible targets only when the secondary effect triggers")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_STING;
    bool32 secondary = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonHitMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 trigger = 0; trigger < 2; trigger++)
                PARAMETRIZE { move = sPoisonHitMoves[m]; user = side ? opponent : player; target = side ? player : opponent; secondary = trigger; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: secondary); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        if (secondary) STATUS_ICON(target, poison: TRUE);
        else NOT STATUS_ICON(target, poison: TRUE);
    } THEN {
        EXPECT_EQ(target->status1, secondary ? STATUS1_POISON : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished poison hit retains Steel damage immunity despite Corrosion")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_STING;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonHitMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sPoisonHitMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(target == player ? SPECIES_BELDUM : SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(target == opponent ? SPECIES_BELDUM : SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } SCENE {
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, move, user);
            HP_BAR(target);
            STATUS_ICON(target, poison: TRUE);
        }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_EQ(target->hp, 500);
    }
}

SINGLE_BATTLE_TEST("Polished poison hit keeps ability prevention even for Corrosion users")
{
    static const enum Ability abilities[] = {ABILITY_IMMUNITY, ABILITY_PASTEL_VEIL, ABILITY_LEAF_GUARD, ABILITY_LEAF_GUARD};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_STING;
    u32 barrier = 0;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonHitMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
                PARAMETRIZE { move = sPoisonHitMoves[m]; user = side ? opponent : player; target = side ? player : opponent; barrier = a; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Ability(user == player ? ABILITY_CORROSION : abilities[barrier]); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Ability(user == opponent ? ABILITY_CORROSION : abilities[barrier]); }
    } WHEN {
        if (barrier == 2) TURN { MOVE(user, MOVE_SUNNY_DAY); }
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
        if (barrier == 3) STATUS_ICON(target, poison: TRUE);
        else NOT STATUS_ICON(target, poison: TRUE);
    } THEN {
        EXPECT_EQ(target->status1, barrier == 3 ? STATUS1_POISON : STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished poison hit retains Protect Safeguard Substitute existing status and misses")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_STING;
    u32 gate = 0;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonHitMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 entry = 0; entry < 5; entry++)
                PARAMETRIZE { move = sPoisonHitMoves[m]; user = side ? opponent : player; target = side ? player : opponent; gate = entry; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Attack(50); SpAttack(50); Defense(200); SpDefense(200); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); if (target == player && gate == 3) Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Attack(50); SpAttack(50); Defense(200); SpDefense(200); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); if (target == opponent && gate == 3) Status1(STATUS1_BURN); }
    } WHEN {
        if (gate == 1) TURN { MOVE(target, MOVE_SAFEGUARD); }
        if (gate == 2) TURN { MOVE(target, MOVE_SUBSTITUTE); }
        if (gate == 4) TURN { MOVE(target, MOVE_DOUBLE_TEAM); }
        TURN { if (gate == 0) MOVE(target, MOVE_PROTECT); MOVE(user, move, hit: gate != 4, secondaryEffect: TRUE); }
    } THEN {
        EXPECT_EQ(target->status1, gate == 3 ? STATUS1_BURN : STATUS1_NONE);
        if (gate == 0 || gate == 4) EXPECT_EQ(target->hp, 500);
        if (gate == 2) {
            EXPECT_EQ(target->hp, 375);
            EXPECT_LT((u32)target->volatiles.substituteHP, 125);
            EXPECT_GT((u32)target->volatiles.substituteHP, 0);
        }
    }
}

SINGLE_BATTLE_TEST("Polished poison hit respects suppression of Corrosion")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_STING;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonHitMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sPoisonHitMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(target == player ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(target == opponent ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_GASTRO_ACID); }
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_NONE);
        EXPECT_LT(target->hp, 500);
    }
}

SINGLE_BATTLE_TEST("Polished poison hit restriction does not affect Toxic or PoisonPowder")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_TOXIC;
    enum Species species = SPECIES_NIDORAN_F;
    for (u32 m = 0; m < 2; m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 type = 0; type < 2; type++)
                PARAMETRIZE { move = m ? MOVE_POISONPOWDER : MOVE_TOXIC; user = side ? opponent : player; target = side ? player : opponent; species = type ? SPECIES_BELDUM : SPECIES_NIDORAN_F; }
    GIVEN {
        PLAYER(target == player ? species : SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(target == opponent ? species : SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE); }
    } THEN {
        EXPECT_EQ(target->status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON), move == MOVE_TOXIC ? STATUS1_TOXIC_POISON : STATUS1_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished poison hit leaves optional damaging poison moves native")
{
    static const enum Move optionalMoves[] = {MOVE_TWINEEDLE, MOVE_SLUDGE, MOVE_SLUDGE_WAVE};
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_TWINEEDLE;
    for (u32 m = 0; m < ARRAY_COUNT(optionalMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = optionalMoves[m]; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(target == player ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(target == opponent ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_POISON);
    }
}

DOUBLE_BATTLE_TEST("Polished poison hit leaves doubles native")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    enum Move move = MOVE_POISON_STING;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonHitMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sPoisonHitMoves[m]; user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; }
    GIVEN {
        PLAYER(target == playerLeft ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Ability(user == playerLeft ? ABILITY_CORROSION : ABILITY_NONE); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(target == opponentLeft ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Ability(user == opponentLeft ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move, target: target, hit: TRUE, secondaryEffect: TRUE); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished poison hit leaves either active Terastal native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_STING;
    bool32 userTera = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonHitMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 tera = 0; tera < 2; tera++)
                PARAMETRIZE { move = sPoisonHitMoves[m]; user = side ? opponent : player; target = side ? player : opponent; userTera = tera; }
    GIVEN {
        PLAYER(target == player ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(100); MaxHP(500); HP(500); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); TeraType(target == player ? TYPE_POISON : TYPE_NORMAL); }
        OPPONENT(target == opponent ? SPECIES_NIDORAN_F : SPECIES_WOBBUFFET) { Speed(50); MaxHP(500); HP(500); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); TeraType(target == opponent ? TYPE_POISON : TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(user, move, hit: TRUE, secondaryEffect: TRUE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(target, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_POISON);
    }
}

SINGLE_BATTLE_TEST("Polished poison hit leaves dynamically retyped moves native")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_POISON_STING;
    enum Species species = SPECIES_NIDORAN_F;
    for (u32 m = 0; m < ARRAY_COUNT(sPoisonHitMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 type = 0; type < 2; type++)
                PARAMETRIZE { move = sPoisonHitMoves[m]; user = side ? opponent : player; target = side ? player : opponent; species = type ? SPECIES_BELDUM : SPECIES_NIDORAN_F; }
    GIVEN {
        PLAYER(target == player ? species : SPECIES_WOBBUFFET) { Speed(target == player ? 100 : 50); MaxHP(500); HP(500); Ability(user == player ? ABILITY_CORROSION : ABILITY_NONE); }
        OPPONENT(target == opponent ? species : SPECIES_WOBBUFFET) { Speed(target == opponent ? 100 : 50); MaxHP(500); HP(500); Ability(user == opponent ? ABILITY_CORROSION : ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_ELECTRIFY); MOVE(user, move, hit: TRUE, secondaryEffect: TRUE); }
    } THEN {
        EXPECT_EQ(target->status1, STATUS1_POISON);
        EXPECT_LT(target->hp, 500);
    }
}
