#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished damage closure Guts includes statused physical attacks and excludes special or suppressed boosts", s16 damage)
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    static const u32 statuses[] = {STATUS1_NONE, STATUS1_BURN, STATUS1_POISON, STATUS1_PARALYSIS, 4, STATUS1_BURN, STATUS1_POISON, STATUS1_BURN};
    for (u32 side = 0; side < 2; side++)
        for (u32 state = 0; state < ARRAY_COUNT(statuses); state++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = state; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == player ? ABILITY_GUTS : kind == 5 || kind == 6 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE); if (user == player) { Status1(statuses[kind]); if (kind == 4) Moves(MOVE_TACKLE, MOVE_SLEEP_TALK); else Moves(MOVE_TACKLE, MOVE_SLEEP_TALK, MOVE_EMBER); } }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == opponent ? ABILITY_GUTS : kind == 5 || kind == 6 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE); if (user == opponent) { Status1(statuses[kind]); if (kind == 4) Moves(MOVE_TACKLE, MOVE_SLEEP_TALK); else Moves(MOVE_TACKLE, MOVE_SLEEP_TALK, MOVE_EMBER); } }
    } WHEN {
        TURN {
            if (kind == 4) MOVE(user, MOVE_SLEEP_TALK, criticalHit: FALSE, WITH_RNG(RNG_PARALYSIS, FALSE));
            else MOVE(user, kind == 7 ? MOVE_EMBER : MOVE_TACKLE, criticalHit: FALSE, WITH_RNG(RNG_PARALYSIS, FALSE));
            MOVE(target, MOVE_SPLASH);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, kind == 7 ? MOVE_EMBER : MOVE_TACKLE, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(results[i].damage, kind >= 1 && kind <= 4 ? 32 : kind == 5 ? 11 : 22);
    } FINALLY {
        for (u32 index = 0; index < 2 * ARRAY_COUNT(statuses); index += ARRAY_COUNT(statuses))
        {
            EXPECT_GT(results[index + 1].damage, results[index].damage);
            for (u32 state = 2; state <= 4; state++) EXPECT_EQ(results[index + 1].damage, results[index + state].damage);
            EXPECT_LT(results[index + 5].damage, results[index].damage);
            EXPECT_EQ(results[index + 6].damage, results[index].damage);
            EXPECT_EQ(results[index + 7].damage, results[index].damage);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage closure Analytic follows actual prior actions including priority switching and cancellation", s16 damage)
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 state = 0; state < 6; state++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = state; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); Defense(73); MaxHP(1000); HP(1000); Speed(user == player ? 100 : kind == 1 || kind == 3 || kind == 5 ? 800 : 50); Ability(user == player ? ABILITY_ANALYTIC : kind == 3 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE); if (target == player && kind == 5) Status1(STATUS1_PARALYSIS); }
        PLAYER(SPECIES_DITTO) { Ability(ABILITY_NONE); Speed(50); Defense(73); MaxHP(1000); HP(1000); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); Defense(73); MaxHP(1000); HP(1000); Speed(user == opponent ? 100 : kind == 1 || kind == 3 || kind == 5 ? 800 : 50); Ability(user == opponent ? ABILITY_ANALYTIC : kind == 3 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE); if (target == opponent && kind == 5) Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_DITTO) { Ability(ABILITY_NONE); Speed(50); Defense(73); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN {
            if (kind == 4) SWITCH(target, 1);
            else MOVE(target, kind == 2 ? MOVE_QUICK_ATTACK : MOVE_SPLASH, WITH_RNG(RNG_PARALYSIS, TRUE));
            MOVE(user, MOVE_TACKLE, criticalHit: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0));
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        if (B_POLISHED_MOVE_BALANCE) EXPECT_EQ(results[i].damage, kind == 0 || kind == 3 ? 26 : 33);
    } FINALLY {
        for (u32 index = 0; index < 12; index += 6)
        {
            EXPECT_EQ(results[index].damage, results[index + 3].damage);
            for (u32 state = 1; state < 6; state++)
                if (state != 3) EXPECT_GT(results[index + state].damage, results[index].damage);
        }
    }
}

// Pinned ApplyDamageAbilities runs on the numerator before Defense, /50 and
// the constant two. These live boundaries complement the raw-stat vectors.
SINGLE_BATTLE_TEST("Polished damage closure pinch abilities check exact HP type and suppression", s16 damage)
{
    static const enum Ability abilities[] = {ABILITY_OVERGROW, ABILITY_BLAZE, ABILITY_TORRENT, ABILITY_SWARM};
    static const enum Move moves[] = {MOVE_VINE_WHIP, MOVE_EMBER, MOVE_WATER_GUN, MOVE_BUG_BITE};
    static const u32 powers[] = {45, 40, 40, 60};
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0, state = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
            for (u32 s = 0; s < 4; s++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = a; state = s; }
    enum Move move = state == 2 ? MOVE_TACKLE : moves[kind];
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(user == player ? 301 : 1000); HP(user == player ? (state == 1 ? 101 : 100) : 1000); Ability(user == player ? abilities[kind] : (state == 3 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(user == opponent ? 301 : 1000); HP(user == opponent ? (state == 1 ? 101 : 100) : 1000); Ability(user == opponent ? abilities[kind] : (state == 3 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); }
    } WHEN {
        TURN { MOVE(user, move, criticalHit: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        if (B_POLISHED_MOVE_BALANCE)
        {
            u32 power = state == 2 ? 40 : powers[kind];
            u32 numerator = 22 * power * 101;
            if (state == 0)
                numerator = numerator * 3 / 2;
            EXPECT_EQ(results[i].damage, numerator / 73 / 50 + 2);
        }
    } FINALLY {
        for (u32 index = 0; index < 2 * ARRAY_COUNT(abilities) * 4; index += 4)
        {
            EXPECT_GT(results[index].damage, results[index + 1].damage);
            EXPECT_EQ(results[index + 1].damage, results[index + 3].damage);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage closure Rivalry preserves both genderless branches and exact fractions", s16 damage)
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 state = 0; state < 6; state++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = state; }
    enum Species userSpecies = kind == 2 || kind == 4 ? SPECIES_MEW : SPECIES_WOBBUFFET;
    enum Species targetSpecies = kind == 3 || kind == 4 ? SPECIES_MEW : SPECIES_WOBBUFFET;
    static const u32 expected[] = {32, 20, 26, 26, 26, 26};
    GIVEN {
        PLAYER(user == player ? userSpecies : targetSpecies) { Level(50); Attack(101); Defense(73); MaxHP(1000); HP(1000); Ability(user == player ? ABILITY_RIVALRY : (kind == 5 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); if ((user == player ? userSpecies : targetSpecies) != SPECIES_MEW) Gender(user == player || kind != 1 ? MON_MALE : MON_FEMALE); }
        OPPONENT(user == opponent ? userSpecies : targetSpecies) { Level(50); Attack(101); Defense(73); MaxHP(1000); HP(1000); Ability(user == opponent ? ABILITY_RIVALRY : (kind == 5 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); if ((user == opponent ? userSpecies : targetSpecies) != SPECIES_MEW) Gender(user == opponent || kind != 1 ? MON_MALE : MON_FEMALE); }
    } WHEN {
        TURN { MOVE(user, MOVE_TACKLE, criticalHit: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(results[i].damage, expected[kind]);
    }
}

SINGLE_BATTLE_TEST("Polished damage closure Sand Force checks weather type Gas and Cloud Nine but not Umbrella", s16 damage)
{
    static const enum Move moves[] = {MOVE_MUD_SLAP, MOVE_ROCK_THROW, MOVE_BULLET_PUNCH, MOVE_TACKLE};
    static const u32 powers[] = {20, 50, 40, 40};
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0, state = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 type = 0; type < ARRAY_COUNT(moves); type++)
            for (u32 s = 0; s < 5; s++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = type; state = s; }
    enum Ability targetAbility = state == 2 ? ABILITY_CLOUD_NINE : state == 3 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE;
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == player ? ABILITY_SAND_FORCE : targetAbility); if (user == player && state == 4) Item(ITEM_UTILITY_UMBRELLA); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == opponent ? ABILITY_SAND_FORCE : targetAbility); if (user == opponent && state == 4) Item(ITEM_UTILITY_UMBRELLA); }
    } WHEN {
        if (state != 0)
            TURN { MOVE(user, MOVE_SANDSTORM); MOVE(target, MOVE_SPLASH); }
        TURN { MOVE(user, moves[kind], hit: TRUE, criticalHit: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, moves[kind], user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        if (B_POLISHED_MOVE_BALANCE)
        {
            u32 numerator = 22 * powers[kind] * 101;
            if (kind < 3 && (state == 1 || state == 4))
                numerator = numerator * 13 / 10;
            EXPECT_EQ(results[i].damage, numerator / 73 / 50 + 2);
        }
    } FINALLY {
        for (u32 index = 0; index < 2 * ARRAY_COUNT(moves) * 5; index += 5)
        {
            EXPECT_EQ(results[index].damage, results[index + 2].damage);
            EXPECT_EQ(results[index].damage, results[index + 3].damage);
            EXPECT_EQ(results[index + 1].damage, results[index + 4].damage);
            if ((index / 5) % ARRAY_COUNT(moves) < 3)
                EXPECT_GT(results[index + 1].damage, results[index].damage);
            else
                EXPECT_EQ(results[index + 1].damage, results[index].damage);
        }
    }
}

SINGLE_BATTLE_TEST("Polished damage closure Tough Claws uses the source Protective Pads contact gate", s16 damage)
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 state = 0; state < 5; state++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = state; }
    enum Item item = kind == 1 ? ITEM_PROTECTIVE_PADS : kind == 2 ? ITEM_PUNCHING_GLOVE : ITEM_NONE;
    enum Move move = kind == 3 ? MOVE_EMBER : MOVE_MACH_PUNCH;
    const u32 expected[] = {66, B_POLISHED_MOVE_BALANCE ? 52 : 66, 56, 26, 52};
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == player ? ABILITY_TOUGH_CLAWS : (kind == 4 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); if (user == player) Item(item); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == opponent ? ABILITY_TOUGH_CLAWS : (kind == 4 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); if (user == opponent) Item(item); }
    } WHEN {
        TURN { MOVE(user, move, criticalHit: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(results[i].damage, expected[kind]);
    }
}

SINGLE_BATTLE_TEST("Polished damage closure Steely Spirit checks current move type and effective ability", s16 damage)
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 state = 0; state < 3; state++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = state; }
    enum Move move = kind == 1 ? MOVE_TACKLE : MOVE_BULLET_PUNCH;
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); Defense(73); MaxHP(1000); HP(1000); Ability(user == player ? ABILITY_STEELY_SPIRIT : (kind == 2 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_DITTO) { Level(50); Attack(101); Defense(73); MaxHP(1000); HP(1000); Ability(user == opponent ? ABILITY_STEELY_SPIRIT : (kind == 2 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); }
    } WHEN {
        TURN { MOVE(user, move, criticalHit: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(results[i].damage, kind == 0 ? 38 : 26);
    }
}

SINGLE_BATTLE_TEST("Polished damage closure Galvanize boosts only converted moves before STAB under older native presets", s16 damage)
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 state = 0; state < 3; state++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = state; }
    enum Move move = kind == 1 ? MOVE_THUNDER_SHOCK : MOVE_TACKLE;
    const u32 expected[] = {B_POLISHED_MOVE_BALANCE ? 46 : 49, 39, 26};
    GIVEN {
        WITH_CONFIG(B_ATE_MULTIPLIER, GEN_6);
        PLAYER(user == player ? SPECIES_PIKACHU : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == player ? ABILITY_GALVANIZE : (kind == 2 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); }
        OPPONENT(user == opponent ? SPECIES_PIKACHU : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == opponent ? ABILITY_GALVANIZE : (kind == 2 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); }
    } WHEN {
        TURN { MOVE(user, move, criticalHit: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(results[i].damage, expected[kind]);
    }
}

SINGLE_BATTLE_TEST("Polished damage closure Adaptability modifies STAB after the base constant and respects Gas", s16 damage)
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 state = 0; state < 3; state++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = state; }
    enum Move move = kind == 1 ? MOVE_TACKLE : MOVE_THUNDER_SHOCK;
    static const u32 expected[] = {52, 26, 39};
    GIVEN {
        PLAYER(user == player ? SPECIES_PIKACHU : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == player ? ABILITY_ADAPTABILITY : (kind == 2 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); }
        OPPONENT(user == opponent ? SPECIES_PIKACHU : SPECIES_DITTO) { Level(50); Attack(101); SpAttack(101); Defense(73); SpDefense(73); MaxHP(1000); HP(1000); Ability(user == opponent ? ABILITY_ADAPTABILITY : (kind == 2 ? ABILITY_NEUTRALIZING_GAS : ABILITY_NONE)); }
    } WHEN {
        TURN { MOVE(user, move, criticalHit: FALSE, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); MOVE(target, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(results[i].damage, expected[kind]);
    }
}
