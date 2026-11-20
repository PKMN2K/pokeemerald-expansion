#include "global.h"
#include "test/battle.h"
#include "polished_attack_vectors.h"

// This first matrix is dispatch/PP coverage, NOT complete effect parity. Some
// moves deliberately fail their prerequisites in this neutral opening turn.
SINGLE_BATTLE_TEST("Polished attack suite dispatches every source move on either side")
{
    enum Move move = MOVE_NONE;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sPolishedAttackVectors); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sPolishedAttackVectors[m].move; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(user == player ? 200 : 100); Ability(ABILITY_NONE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            MovesWithPP({user == player ? move : MOVE_SPLASH, 5});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(user == opponent ? 200 : 100); Ability(ABILITY_NONE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
            MovesWithPP({user == opponent ? move : MOVE_SPLASH, 5});
        }
    } WHEN {
        TURN {
            if (move == MOVE_METRONOME)
                MOVE(user, move, WITH_RNG(RNG_METRONOME, B_POLISHED_MOVE_BALANCE ? 150 : MOVE_SPLASH));
            else
                MOVE(user, move, secondaryEffect: FALSE);
            MOVE(target, MOVE_SPLASH);
        }
    } THEN {
        // Transform replaces the live slots with five PP; Struggle spends none.
        EXPECT_EQ(user->pp[0], move == MOVE_TRANSFORM || move == MOVE_STRUGGLE ? 5 : 4);
    }
}

SINGLE_BATTLE_TEST("Polished attack suite unblocked source attacks actually damage either target")
{
    enum Move move = MOVE_NONE;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sPolishedAttackVectors); m++)
        if (sPolishedAttackVectors[m].directDamage)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { move = sPolishedAttackVectors[m].move; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Speed(user == player ? 200 : 100); Ability(ABILITY_NONE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Speed(user == opponent ? 200 : 100); Ability(ABILITY_NONE);
            MaxHP(1000); HP(1000); Attack(10); SpAttack(10);
        }
    } WHEN {
        TURN { MOVE(user, move, secondaryEffect: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
    } THEN {
        EXPECT_LT(target->hp, 1000);
    }
}

SINGLE_BATTLE_TEST("Polished attack suite source stat changes and limits work on either side")
{
    const struct PolishedStatVector *vector = &sPolishedStatVectors[0];
    struct BattlePokemon *user = player, *target = opponent;
    u32 turns = 1;
    for (u32 m = 0; m < ARRAY_COUNT(sPolishedStatVectors); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 count = 1; count <= 7; count += 6)
                PARAMETRIZE { vector = &sPolishedStatVectors[m]; user = side ? opponent : player; target = side ? player : opponent; turns = count; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        for (u32 n = 0; n < turns; n++)
            TURN { MOVE(user, vector->move); }
    } THEN {
        struct BattlePokemon *changed = vector->opponent ? target : user;
        struct BattlePokemon *unchanged = vector->opponent ? user : target;
        for (u32 stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
        {
            s32 expected = DEFAULT_STAT_STAGE + vector->deltas[stat] * (s32)turns;
            EXPECT_EQ(changed->statStages[stat], min(MAX_STAT_STAGE, max(MIN_STAT_STAGE, expected)));
            EXPECT_EQ(unchanged->statStages[stat], DEFAULT_STAT_STAGE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished attack suite Protect blocks every ordinary direct source attack")
{
    enum Move move = MOVE_NONE;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sPolishedAttackVectors); m++)
        if (sPolishedAttackVectors[m].directDamage)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { move = sPolishedAttackVectors[m].move; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_PROTECT); MOVE(user, move, secondaryEffect: TRUE); }
    } THEN {
        EXPECT_EQ(target->hp, 1000);
        EXPECT_EQ(target->status1, STATUS1_NONE);
        for (u32 stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
            EXPECT_EQ(target->statStages[stat], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished attack suite Substitute intercepts direct attacks except source sound moves")
{
    enum Move move = MOVE_NONE;
    bool32 sound = FALSE;
    struct BattlePokemon *user = player, *target = opponent;
    for (u32 m = 0; m < ARRAY_COUNT(sPolishedAttackVectors); m++)
        if (sPolishedAttackVectors[m].directDamage)
            for (u32 side = 0; side < 2; side++)
                PARAMETRIZE { move = sPolishedAttackVectors[m].move; sound = sPolishedAttackVectors[m].sound; user = side ? opponent : player; target = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, move, secondaryEffect: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, target);
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        if (sound) HP_BAR(target);
        else SUB_HIT(target);
    } THEN {
        if (sound) EXPECT_LT(target->hp, 750);
        else EXPECT_EQ(target->hp, 750);
    }
}

SINGLE_BATTLE_TEST("Polished attack suite half HP recovery floors odd maximum HP")
{
    enum Move move = MOVE_RECOVER;
    struct BattlePokemon *user = player;
    u32 maxHP = 201;
    PARAMETRIZE { move = MOVE_RECOVER; user = player; }
    PARAMETRIZE { move = MOVE_RECOVER; user = opponent; }
    PARAMETRIZE { move = MOVE_ROOST; user = player; }
    PARAMETRIZE { move = MOVE_ROOST; user = opponent; }
    PARAMETRIZE { move = MOVE_POLISHED_FRESH_SNACK; user = player; }
    PARAMETRIZE { move = MOVE_POLISHED_FRESH_SNACK; user = opponent; }
    GIVEN {
        WITH_CONFIG(B_HALF_HP_HEALING_ROUNDING, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(maxHP); HP(1); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(maxHP); HP(1); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move); }
    } THEN {
        EXPECT_EQ(user->hp, B_POLISHED_MOVE_BALANCE || move == MOVE_POLISHED_FRESH_SNACK ? 101 : 102);
    }
}

static const enum Move sRecoilMoves[] = {MOVE_TAKE_DOWN, MOVE_DOUBLE_EDGE, MOVE_BRAVE_BIRD, MOVE_WILD_CHARGE, MOVE_FLARE_BLITZ};

SINGLE_BATTLE_TEST("Polished attack suite recoil uses source fractions and exact division")
{
    enum Move move = MOVE_TAKE_DOWN;
    struct BattlePokemon *user = player, *target = opponent;
    u32 attack = 40;
    s16 damage;
    for (u32 m = 0; m < ARRAY_COUNT(sRecoilMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 40; a <= 200; a += 40)
                PARAMETRIZE { move = sRecoilMoves[m]; user = side ? opponent : player; target = side ? player : opponent; attack = a; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(attack); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Attack(attack); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move, secondaryEffect: FALSE); }
    } SCENE {
        HP_BAR(target, captureDamage: &damage);
        HP_BAR(user);
    } THEN {
        u32 expected = B_POLISHED_MOVE_BALANCE
            ? damage / (move == MOVE_DOUBLE_EDGE || move == MOVE_FLARE_BLITZ ? 3 : 4)
            : damage * GetMoveRecoil(move) / 100;
        EXPECT_EQ(user->hp, 1000 - max(1, expected));
    }
}

static const enum Move sDrainMoves[] = {MOVE_ABSORB, MOVE_MEGA_DRAIN, MOVE_LEECH_LIFE, MOVE_DRAINING_KISS, MOVE_DRAIN_PUNCH, MOVE_GIGA_DRAIN};

SINGLE_BATTLE_TEST("Polished attack suite drain applies source Kiss and Big Root arithmetic in order")
{
    enum Move move = MOVE_ABSORB;
    struct BattlePokemon *user = player, *target = opponent;
    u32 attack = 40;
    bool32 root = FALSE;
    bool32 ooze = FALSE;
    s16 damage;
    for (u32 m = 0; m < ARRAY_COUNT(sDrainMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 40; a <= 200; a += 40)
                for (u32 item = 0; item < 2; item++)
                    for (u32 ability = 0; ability < 2; ability++)
                        PARAMETRIZE { move = sDrainMoves[m]; user = side ? opponent : player; target = side ? player : opponent; attack = a; root = item; ooze = ability; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(100); Attack(attack); SpAttack(attack); Ability(user == player || !ooze ? ABILITY_NONE : ABILITY_LIQUID_OOZE); Item(root ? ITEM_BIG_ROOT : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(100); Attack(attack); SpAttack(attack); Ability(user == opponent || !ooze ? ABILITY_NONE : ABILITY_LIQUID_OOZE); Item(root ? ITEM_BIG_ROOT : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(user, move); }
    } SCENE {
        HP_BAR(target, captureDamage: &damage);
        HP_BAR(user);
    } THEN {
        u32 expected;
        if (B_POLISHED_MOVE_BALANCE)
        {
            expected = move == MOVE_DRAINING_KISS ? damage + damage / 2 : damage;
            if (root) expected = expected * 13 / 10;
            expected = max(1, expected / 2);
        }
        else
        {
            expected = max(1, damage * (move == MOVE_DRAINING_KISS ? 75 : 50) / 100);
            if (root) expected = expected * 13 / 10;
        }
        EXPECT_EQ(user->hp, ooze ? 100 - expected : 100 + expected);
    }
}

SINGLE_BATTLE_TEST("Polished attack suite recoil remains blocked by Rock Head and Magic Guard")
{
    enum Move move = MOVE_TAKE_DOWN;
    enum Ability ability = ABILITY_ROCK_HEAD;
    struct BattlePokemon *user = player;
    for (u32 m = 0; m < ARRAY_COUNT(sRecoilMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < 2; a++)
                PARAMETRIZE { move = sRecoilMoves[m]; user = side ? opponent : player; ability = a ? ABILITY_MAGIC_GUARD : ABILITY_ROCK_HEAD; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ability); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Ability(ability); }
    } WHEN {
        TURN { MOVE(user, move, secondaryEffect: FALSE); }
    } THEN {
        EXPECT_EQ(user->hp, 1000);
    }
}

SINGLE_BATTLE_TEST("Polished attack suite recovery still fails at full HP and optional moves retain rounding")
{
    enum Move move = MOVE_RECOVER;
    struct BattlePokemon *user = player;
    bool32 full = TRUE;
    for (u32 side = 0; side < 2; side++)
    {
        PARAMETRIZE { move = MOVE_RECOVER; user = side ? opponent : player; }
        PARAMETRIZE { move = MOVE_ROOST; user = side ? opponent : player; }
        PARAMETRIZE { move = MOVE_POLISHED_FRESH_SNACK; user = side ? opponent : player; }
        PARAMETRIZE { move = MOVE_SLACK_OFF; user = side ? opponent : player; full = FALSE; }
    }
    GIVEN {
        WITH_CONFIG(B_HALF_HP_HEALING_ROUNDING, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(201); HP(full ? 201 : 1); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(201); HP(full ? 201 : 1); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, move); }
    } THEN {
        EXPECT_EQ(user->hp, full ? 201 : 102);
    }
}

static const enum Move sArithmeticControls[] = {MOVE_RECOVER, MOVE_BRAVE_BIRD, MOVE_DRAINING_KISS};

DOUBLE_BATTLE_TEST("Polished attack suite optional doubles retain native recovery recoil and drain")
{
    enum Move move = MOVE_RECOVER;
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    s16 damage;
    for (u32 m = 0; m < ARRAY_COUNT(sArithmeticControls); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sArithmeticControls[m]; user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; }
    GIVEN {
        WITH_CONFIG(B_HALF_HP_HEALING_ROUNDING, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1001); HP(100); Ability(ABILITY_NONE); Item(ITEM_BIG_ROOT); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1001); HP(100); Ability(ABILITY_NONE); Item(ITEM_BIG_ROOT); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN {
            if (move == MOVE_RECOVER) MOVE(user, move);
            else MOVE(user, move, target: target, secondaryEffect: FALSE);
        }
    } SCENE {
        if (move != MOVE_RECOVER) HP_BAR(target, captureDamage: &damage);
        HP_BAR(user);
    } THEN {
        if (move == MOVE_RECOVER) EXPECT_EQ(user->hp, 601);
        else if (move == MOVE_BRAVE_BIRD) EXPECT_EQ(user->hp, 100 - max(1, damage * GetMoveRecoil(move) / 100));
        else EXPECT_EQ(user->hp, 100 + max(1, (damage * 75 / 100) * 13 / 10));
    }
}

SINGLE_BATTLE_TEST("Polished attack suite either active gimmick retains native recovery recoil and drain")
{
    enum Move move = MOVE_RECOVER;
    struct BattlePokemon *user = player, *target = opponent;
    bool32 userTera = FALSE;
    s16 damage;
    for (u32 m = 0; m < ARRAY_COUNT(sArithmeticControls); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 tera = 0; tera < 2; tera++)
                PARAMETRIZE { move = sArithmeticControls[m]; user = side ? opponent : player; target = side ? player : opponent; userTera = tera; }
    GIVEN {
        WITH_CONFIG(B_HALF_HP_HEALING_ROUNDING, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1001); HP(100); Ability(ABILITY_NONE); Item(ITEM_BIG_ROOT); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1001); HP(100); Ability(ABILITY_NONE); Item(ITEM_BIG_ROOT); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(user, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(target, MOVE_CELEBRATE, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA); }
        TURN { MOVE(user, move, secondaryEffect: FALSE); }
    } SCENE {
        if (move != MOVE_RECOVER) HP_BAR(target, captureDamage: &damage);
        HP_BAR(user);
    } THEN {
        if (move == MOVE_RECOVER) EXPECT_EQ(user->hp, 601);
        else if (move == MOVE_BRAVE_BIRD) EXPECT_EQ(user->hp, 100 - max(1, damage * GetMoveRecoil(move) / 100));
        else EXPECT_EQ(user->hp, 100 + max(1, (damage * 75 / 100) * 13 / 10));
    }
}
