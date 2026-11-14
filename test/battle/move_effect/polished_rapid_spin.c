#include "global.h"
#include "test/battle.h"
#include "battle_util.h"

ASSUMPTIONS
{
    ASSUME(B_SPEED_BUFFING_RAPID_SPIN >= GEN_8);
    ASSUME(IsSheerForceAffected(MOVE_RAPID_SPIN, ABILITY_SHEER_FORCE));
}

static bool32 ClearsSpinEffects(enum Ability ability)
{
    return B_POLISHED_MOVE_BALANCE || ability != ABILITY_SHEER_FORCE;
}

SINGLE_BATTLE_TEST("Polished Rapid Spin removes every source hazard layer without clearing the opposite side")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    enum Move hazard = MOVE_SPIKES;
    u32 layers = 1, side;
    for (u32 n = 0; n < 2; n++)
        for (u32 sheer = 0; sheer < 2; sheer++)
            for (u32 kind = 0; kind < 2; kind++)
                for (u32 maximum = 0; maximum < 2; maximum++)
                    PARAMETRIZE { user = n ? opponent : player; target = n ? player : opponent; ability = sheer ? ABILITY_SHEER_FORCE : ABILITY_NONE; hazard = kind ? MOVE_TOXIC_SPIKES : MOVE_SPIKES; layers = maximum ? (kind ? 2 : 3) : 1; }
    side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : 3000); HP(user == player ? 3000 : 3000);
            Item(user == player ? ITEM_NONE : ITEM_NONE); Status1(user == player ? STATUS1_NONE : STATUS1_NONE);
            if (user == player) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : 3000); HP(user == opponent ? 3000 : 3000);
            Item(user == opponent ? ITEM_NONE : ITEM_NONE); Status1(user == opponent ? STATUS1_NONE : STATUS1_NONE);
            if (user == opponent) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
    } WHEN {
        TURN { MOVE(user, MOVE_SPIKES); MOVE(target, hazard); }
        for (u32 i = 1; i < layers; i++)
            TURN { MOVE(target, hazard); }
        TURN { MOVE(user, MOVE_RAPID_SPIN); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        HP_BAR(target);
    } THEN {
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, !ClearsSpinEffects(ability) && hazard == MOVE_SPIKES ? layers : 0);
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, !ClearsSpinEffects(ability) && hazard == MOVE_TOXIC_SPIKES ? layers : 0);
        EXPECT_EQ(gBattleStruct->numHazards[side], ClearsSpinEffects(ability) ? 0 : 1);
        EXPECT_EQ((u32)gSideTimers[side ^ 1].spikesAmount, 1);
        EXPECT_EQ(gBattleStruct->numHazards[side ^ 1], 1);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (ability != ABILITY_SHEER_FORCE));
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Rapid Spin clears binding Leech Seed and both stacked hazards together")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 sheer = 0; sheer < 2; sheer++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = sheer ? ABILITY_SHEER_FORCE : ABILITY_NONE; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 200);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : 3000); HP(user == player ? 3000 : 3000);
            Item(user == player ? ITEM_NONE : ITEM_NONE); Status1(user == player ? STATUS1_NONE : STATUS1_NONE);
            if (user == player) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 200);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : 3000); HP(user == opponent ? 3000 : 3000);
            Item(user == opponent ? ITEM_NONE : ITEM_NONE); Status1(user == opponent ? STATUS1_NONE : STATUS1_NONE);
            if (user == opponent) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
    } WHEN {
        for (u32 i = 0; i < 3; i++) TURN { MOVE(target, MOVE_SPIKES); }
        for (u32 i = 0; i < 2; i++) TURN { MOVE(target, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(target, MOVE_WRAP, hit: TRUE, WITH_RNG(RNG_WRAP, 5)); }
        TURN { MOVE(target, MOVE_LEECH_SEED, hit: TRUE); }
        TURN { MOVE(target, MOVE_SPIKES); MOVE(user, MOVE_RAPID_SPIN); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        HP_BAR(target);
        if (ClearsSpinEffects(ability)) NOT HP_BAR(user);
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)user->volatiles.wrapped, !ClearsSpinEffects(ability));
        EXPECT_EQ(user->volatiles.leechSeed != 0, !ClearsSpinEffects(ability));
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, ClearsSpinEffects(ability) ? 0 : 3);
        EXPECT_EQ((u32)gSideTimers[side].toxicSpikesAmount, ClearsSpinEffects(ability) ? 0 : 2);
        EXPECT_EQ(gBattleStruct->numHazards[side], ClearsSpinEffects(ability) ? 0 : 2);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (ability != ABILITY_SHEER_FORCE));
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Rapid Spin keeps hazards and Leech Seed after misses protection immunity or paralysis")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    u32 reason = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 sheer = 0; sheer < 2; sheer++)
            for (u32 failure = 0; failure < 4; failure++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = sheer ? ABILITY_SHEER_FORCE : ABILITY_NONE; reason = failure; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : (reason == 2 ? SPECIES_GASTLY : SPECIES_WOBBUFFET)) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : 3000); HP(user == player ? 3000 : 3000);
            Item(user == player ? ITEM_NONE : ITEM_NONE); Status1(user == player ? (reason == 3 ? STATUS1_PARALYSIS : STATUS1_NONE) : STATUS1_NONE);
            if (user == player) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : (reason == 2 ? SPECIES_GASTLY : SPECIES_WOBBUFFET)) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : 3000); HP(user == opponent ? 3000 : 3000);
            Item(user == opponent ? ITEM_NONE : ITEM_NONE); Status1(user == opponent ? (reason == 3 ? STATUS1_PARALYSIS : STATUS1_NONE) : STATUS1_NONE);
            if (user == opponent) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(target, MOVE_LEECH_SEED, hit: TRUE); }
        if (reason == 0) TURN { MOVE(target, MOVE_SAND_ATTACK, hit: TRUE); }
        if (reason == 0) TURN { MOVE(user, MOVE_RAPID_SPIN, hit: FALSE); }
        else if (reason == 1) TURN { MOVE(target, MOVE_PROTECT); MOVE(user, MOVE_RAPID_SPIN); }
        else if (reason == 2) TURN { MOVE(user, MOVE_RAPID_SPIN); }
        else TURN { MOVE(user, MOVE_RAPID_SPIN, WITH_RNG(RNG_PARALYSIS, TRUE)); }
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, 1);
        EXPECT_EQ(gBattleStruct->numHazards[side], 1);
        EXPECT_NE((u32)user->volatiles.leechSeed, 0);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->pp[0], reason == 3 ? 10 : 9);
        EXPECT_EQ(target->hp, target->maxHP);
    }
}

SINGLE_BATTLE_TEST("Polished Rapid Spin cleanup works through intact and breaking Substitutes")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    bool32 breaks = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 sheer = 0; sheer < 2; sheer++)
            for (u32 small = 0; small < 2; small++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = sheer ? ABILITY_SHEER_FORCE : ABILITY_NONE; breaks = small; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : (breaks ? 100 : 3000)); HP(user == player ? 3000 : (breaks ? 100 : 3000));
            Item(user == player ? ITEM_NONE : ITEM_NONE); Status1(user == player ? STATUS1_NONE : STATUS1_NONE);
            if (user == player) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : (breaks ? 100 : 3000)); HP(user == opponent ? 3000 : (breaks ? 100 : 3000));
            Item(user == opponent ? ITEM_NONE : ITEM_NONE); Status1(user == opponent ? STATUS1_NONE : STATUS1_NONE);
            if (user == opponent) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_RAPID_SPIN, criticalHit: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        SUB_HIT(target);
        if (breaks) ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, target);
        NOT HP_BAR(target);
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, !ClearsSpinEffects(ability));
        EXPECT_EQ((u32)target->volatiles.substitute, !breaks);
        EXPECT_EQ(target->hp, breaks ? 75 : 2250);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + (ability != ABILITY_SHEER_FORCE));
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Rapid Spin cleanup still runs after knocking out the target")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    for (u32 side = 0; side < 2; side++)
        for (u32 sheer = 0; sheer < 2; sheer++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = sheer ? ABILITY_SHEER_FORCE : ABILITY_NONE; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : 3000); HP(user == player ? 3000 : 1);
            Item(user == player ? ITEM_NONE : ITEM_NONE); Status1(user == player ? STATUS1_NONE : STATUS1_NONE);
            if (user == player) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); MaxHP(3000); HP(3000); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : 3000); HP(user == opponent ? 3000 : 1);
            Item(user == opponent ? ITEM_NONE : ITEM_NONE); Status1(user == opponent ? STATUS1_NONE : STATUS1_NONE);
            if (user == opponent) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); MaxHP(3000); HP(3000); }
    } WHEN {
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(user, MOVE_RAPID_SPIN); SEND_OUT(target, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        HP_BAR(target);
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, !ClearsSpinEffects(ability));
        EXPECT_EQ(gBattleStruct->numHazards[side], !ClearsSpinEffects(ability));
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Rapid Spin does not clean hazards after its user faints from contact")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_SHEER_FORCE;
    bool32 helmet = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 item = 0; item < 2; item++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; helmet = item; }
    GIVEN {
        WITH_CONFIG(B_FAINT_MOVE_EFFECT_TIMING, GEN_9);
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : (helmet ? ABILITY_NONE : ABILITY_ROUGH_SKIN)); Speed(user == player ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : 3000); HP(user == player ? 1 : 3000);
            Item(user == player ? ITEM_NONE : (helmet ? ITEM_ROCKY_HELMET : ITEM_NONE)); Status1(user == player ? STATUS1_NONE : STATUS1_NONE);
            if (user == player) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); MaxHP(3000); HP(3000); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : (helmet ? ABILITY_NONE : ABILITY_ROUGH_SKIN)); Speed(user == opponent ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : 3000); HP(user == opponent ? 1 : 3000);
            Item(user == opponent ? ITEM_NONE : (helmet ? ITEM_ROCKY_HELMET : ITEM_NONE)); Status1(user == opponent ? STATUS1_NONE : STATUS1_NONE);
            if (user == opponent) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); MaxHP(3000); HP(3000); }
    } WHEN {
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(user, MOVE_RAPID_SPIN); SEND_OUT(user, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        HP_BAR(target);
        HP_BAR(user, hp: 0);
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, 1);
        EXPECT_EQ(gBattleStruct->numHazards[side], 1);
        EXPECT_LT(user->hp, user->maxHP);
    }
}

SINGLE_BATTLE_TEST("Polished Rapid Spin preserves Sheer Force suppression of Life Orb and Shell Bell")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_SHEER_FORCE;
    enum Item item = ITEM_LIFE_ORB;
    for (u32 side = 0; side < 2; side++)
        for (u32 bell = 0; bell < 2; bell++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; item = bell ? ITEM_SHELL_BELL : ITEM_LIFE_ORB; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : 3000); HP(user == player ? 500 : 3000);
            Item(user == player ? item : ITEM_NONE); Status1(user == player ? STATUS1_NONE : STATUS1_NONE);
            if (user == player) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : 3000); HP(user == opponent ? 500 : 3000);
            Item(user == opponent ? item : ITEM_NONE); Status1(user == opponent ? STATUS1_NONE : STATUS1_NONE);
            if (user == opponent) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(user, MOVE_RAPID_SPIN); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        HP_BAR(target);
        NOT HP_BAR(user);
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, !B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(user->hp, 500);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Rapid Spin keeps ordinary cleanup and Speed when Neutralizing Gas suppresses Sheer Force")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_SHEER_FORCE;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NEUTRALIZING_GAS); Speed(user == player ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : 3000); HP(user == player ? 3000 : 3000);
            Item(user == player ? ITEM_NONE : ITEM_NONE); Status1(user == player ? STATUS1_NONE : STATUS1_NONE);
            if (user == player) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NEUTRALIZING_GAS); Speed(user == opponent ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : 3000); HP(user == opponent ? 3000 : 3000);
            Item(user == opponent ? ITEM_NONE : ITEM_NONE); Status1(user == opponent ? STATUS1_NONE : STATUS1_NONE);
            if (user == opponent) MovesWithPP({MOVE_RAPID_SPIN, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(target, MOVE_LEECH_SEED, hit: TRUE); }
        TURN { MOVE(user, MOVE_RAPID_SPIN); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        HP_BAR(target);
        NOT HP_BAR(user);
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, 0);
        EXPECT_EQ((u32)user->volatiles.leechSeed, 0);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(user->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Rapid Spin repeats cleanup through its last PP and leaves later moves usable")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_SHEER_FORCE;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : 3000); HP(user == player ? 3000 : 3000);
            Item(user == player ? ITEM_NONE : ITEM_NONE); Status1(user == player ? STATUS1_NONE : STATUS1_NONE);
            if (user == player) MovesWithPP({MOVE_RAPID_SPIN, 2}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : 3000); HP(user == opponent ? 3000 : 3000);
            Item(user == opponent ? ITEM_NONE : ITEM_NONE); Status1(user == opponent ? STATUS1_NONE : STATUS1_NONE);
            if (user == opponent) MovesWithPP({MOVE_RAPID_SPIN, 2}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(user, MOVE_RAPID_SPIN); }
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(user, MOVE_RAPID_SPIN); }
        TURN { MOVE(user, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        HP_BAR(target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        HP_BAR(target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, user);
        HP_BAR(target);
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, B_POLISHED_MOVE_BALANCE ? 0 : 2);
        EXPECT_EQ(gBattleStruct->numHazards[side], !B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->pp[0], 0);
        EXPECT_EQ(user->pp[2], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Rapid Spin preserves native Sheer Force cleanup for optional moves and active gimmicks")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_SHEER_FORCE;
    enum Move move = MOVE_MORTAL_SPIN;
    for (u32 side = 0; side < 2; side++)
        for (u32 tera = 0; tera < 2; tera++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = tera ? MOVE_RAPID_SPIN : MOVE_MORTAL_SPIN; }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == player ? ability : ABILITY_NONE); Speed(user == player ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == player ? 3000 : 3000); HP(user == player ? 3000 : 3000);
            Item(user == player ? ITEM_NONE : ITEM_NONE); Status1(user == player ? STATUS1_NONE : STATUS1_NONE);
            if (user == player) MovesWithPP({move, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
            TeraType(TYPE_NORMAL);
        }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : SPECIES_WOBBUFFET) {
            Ability(user == opponent ? ability : ABILITY_NONE); Speed(user == opponent ? 100 : 50);
            Level(50); Attack(300); Defense(100); SpAttack(100); SpDefense(100);
            MaxHP(user == opponent ? 3000 : 3000); HP(user == opponent ? 3000 : 3000);
            Item(user == opponent ? ITEM_NONE : ITEM_NONE); Status1(user == opponent ? STATUS1_NONE : STATUS1_NONE);
            if (user == opponent) MovesWithPP({move, 10}, {MOVE_SPIKES, 10}, {MOVE_TACKLE, 10});
            TeraType(TYPE_NORMAL);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(user, move, gimmick: move == MOVE_RAPID_SPIN ? GIMMICK_TERA : GIMMICK_NONE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        HP_BAR(target);
    } THEN {
        u32 side = user == player ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, 1);
        EXPECT_EQ(gBattleStruct->numHazards[side], 1);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ(target->status1, STATUS1_NONE);
    }
}

DOUBLE_BATTLE_TEST("Polished Rapid Spin retains native Sheer Force behavior in optional double battles")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    PARAMETRIZE { user = playerLeft; target = opponentLeft; }
    PARAMETRIZE { user = opponentLeft; target = playerLeft; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == playerLeft ? ABILITY_SHEER_FORCE : ABILITY_NONE); Speed(100); MaxHP(3000); HP(3000); }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponentLeft ? ABILITY_SHEER_FORCE : ABILITY_NONE); Speed(50); MaxHP(3000); HP(3000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(target, MOVE_SPIKES); }
        TURN { MOVE(user, MOVE_RAPID_SPIN, target: target); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAPID_SPIN, user);
        HP_BAR(target);
    } THEN {
        u32 side = user == playerLeft ? B_SIDE_PLAYER : B_SIDE_OPPONENT;
        EXPECT_EQ((u32)gSideTimers[side].spikesAmount, 1);
        EXPECT_EQ(gBattleStruct->numHazards[side], 1);
        EXPECT_EQ(user->statStages[STAT_SPEED], DEFAULT_STAT_STAGE);
    }
}
