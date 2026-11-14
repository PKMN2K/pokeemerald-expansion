#include "global.h"
#include "test/battle.h"

// Independent expected probabilities: source uses up to six ternary rolls;
// native generation settings cap after three previous successes.
static u32 ProtectDenominator(u32 successes, bool32 source)
{
    u32 denominator = 1;
    u32 cap = source ? 6 : 3;
    u32 base = source || B_PROTECT_FAILURE_RATE >= GEN_5 ? 3 : 2;
    for (u32 i = 0; i < min(successes, cap); i++)
        denominator *= base;
    return denominator;
}

SINGLE_BATTLE_TEST("Polished Protect chain first use protects or endures on both sides")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = kind ? MOVE_ENDURE : MOVE_PROTECT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == player ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == player) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
            if (user == player) HP(1);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == opponent) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
            if (user == opponent) HP(1);
        }
    } WHEN {
        TURN { MOVE(user, move); MOVE(target, MOVE_SONIC_BOOM, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        if (move == MOVE_PROTECT) NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SONIC_BOOM, target);
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_SONIC_BOOM, target);
    } THEN {
        EXPECT_EQ(user->hp, 1);
        EXPECT_EQ(user->pp[0], 9);
        EXPECT_EQ((u32)user->volatiles.consecutiveMoveUses, 1);
    }
}

SINGLE_BATTLE_TEST("Polished Protect chain success probability follows every stage and remains capped")
{
    struct BattlePokemon *user = player;
    enum Move move = MOVE_PROTECT;
    u32 prior = 1;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            for (u32 uses = 1; uses <= 8; uses++)
                if (uses != 7)
                    PARAMETRIZE { user = side ? opponent : player; move = kind ? MOVE_ENDURE : MOVE_PROTECT; prior = uses; }
    PASSES_RANDOMLY(1, ProtectDenominator(prior, B_POLISHED_MOVE_BALANCE), RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == player ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == player) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == opponent) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
    } WHEN {
        for (u32 i = 0; i < prior; i++)
            TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(user, move); }
    } SCENE {
        for (u32 i = 0; i <= prior; i++)
            ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        EXPECT_EQ(user->pp[0], 9 - prior);
    }
}

SINGLE_BATTLE_TEST("Polished Protect chain alternating Protect and Endure shares the probability counter")
{
    struct BattlePokemon *user = player;
    enum Move move = MOVE_PROTECT, other = MOVE_ENDURE;
    u32 prior = 4;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            for (u32 uses = 4; uses <= 6; uses += 2)
                PARAMETRIZE { user = side ? opponent : player; move = kind ? MOVE_ENDURE : MOVE_PROTECT; other = kind ? MOVE_PROTECT : MOVE_ENDURE; prior = uses; }
    PASSES_RANDOMLY(1, ProtectDenominator(prior, B_POLISHED_MOVE_BALANCE), RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == player ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == player) MovesWithPP({move, 10}, {other, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == opponent) MovesWithPP({move, 10}, {other, 10});
        }
    } WHEN {
        for (u32 i = 0; i < prior; i++)
            TURN { MOVE(user, i % 2 ? other : move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(user, move); }
    } SCENE {
        for (u32 i = 0; i < prior; i++)
            ANIMATION(ANIM_TYPE_MOVE, i % 2 ? other : move, user);
        ANIMATION(ANIM_TYPE_MOVE, move, user);
    }
}

SINGLE_BATTLE_TEST("Polished Protect chain saturates the stored count without changing PP costs")
{
    struct BattlePokemon *user = player;
    enum Move move = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; move = kind ? MOVE_ENDURE : MOVE_PROTECT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == player ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == player) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == opponent) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
    } WHEN {
        for (u32 i = 0; i < 10; i++)
            TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
    } SCENE {
        for (u32 i = 0; i < 10; i++)
            ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.consecutiveMoveUses, B_POLISHED_MOVE_BALANCE ? 6 : 10);
        EXPECT_EQ(user->pp[0], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Protect chain failure resets the next attempt to guaranteed success")
{
    struct BattlePokemon *user = player;
    enum Move move = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; move = kind ? MOVE_ENDURE : MOVE_PROTECT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == player ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == player) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == opponent) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
    } WHEN {
        for (u32 i = 0; i < 6; i++)
            TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, B_POLISHED_MOVE_BALANCE ? 0 : 2)); }
        TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, B_POLISHED_MOVE_BALANCE ? 0 : 2)); }
    } SCENE {
        for (u32 i = 0; i < 6; i++)
            ANIMATION(ANIM_TYPE_MOVE, move, user);
        MESSAGE("But it failed!");
        ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.consecutiveMoveUses, 1);
        EXPECT_EQ(user->pp[0], 2);
    }
}

SINGLE_BATTLE_TEST("Polished Protect chain an ordinary selected move resets the next attempt")
{
    struct BattlePokemon *user = player;
    enum Move move = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; move = kind ? MOVE_ENDURE : MOVE_PROTECT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == player ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == player) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == opponent) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
    } WHEN {
        for (u32 i = 0; i < 4; i++)
            TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(user, MOVE_SPLASH); }
        TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, B_POLISHED_MOVE_BALANCE ? 0 : 2)); }
    } SCENE {
        for (u32 i = 0; i < 4; i++)
            ANIMATION(ANIM_TYPE_MOVE, move, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, user);
        ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.consecutiveMoveUses, 1);
        EXPECT_EQ(user->pp[0], 5);
        EXPECT_EQ(user->pp[1], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Protect chain switching and reentry reset the next attempt")
{
    struct BattlePokemon *user = player;
    enum Move move = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; move = kind ? MOVE_ENDURE : MOVE_PROTECT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == player ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == player) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == opponent) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(75); }
    } WHEN {
        for (u32 i = 0; i < 4; i++)
            TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { SWITCH(user, 1); }
        TURN { SWITCH(user, 0); }
        TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, B_POLISHED_MOVE_BALANCE ? 0 : 2)); }
    } SCENE {
        for (u32 i = 0; i < 4; i++)
            ANIMATION(ANIM_TYPE_MOVE, move, user);
        ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.consecutiveMoveUses, 1);
        EXPECT_EQ(user->pp[0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished Protect chain acting after the opponent fails and resets the counter")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move move = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; move = kind ? MOVE_ENDURE : MOVE_PROTECT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == player ? 50 : 100); MaxHP(1000); HP(1000);
            if (user == player) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == opponent ? 50 : 100); MaxHP(1000); HP(1000);
            if (user == opponent) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
    } WHEN {
        for (u32 i = 0; i < 4; i++)
            TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(target, MOVE_PROTECT); MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, B_POLISHED_MOVE_BALANCE ? 0 : 2)); }
    } SCENE {
        for (u32 i = 0; i < 4; i++)
            ANIMATION(ANIM_TYPE_MOVE, move, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, target);
        MESSAGE("But it failed!");
        ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.consecutiveMoveUses, 1);
        EXPECT_EQ(user->pp[0], 4);
    }
}

SINGLE_BATTLE_TEST("Polished Protect chain retains native consecutive odds for optional Detect")
{
    struct BattlePokemon *user = player;
    enum Move move = MOVE_DETECT;
    PARAMETRIZE { user = player; }
    PARAMETRIZE { user = opponent; }
    PASSES_RANDOMLY(1, ProtectDenominator(6, FALSE), RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == player ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == player) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(user == opponent ? 100 : 50); MaxHP(1000); HP(1000);
            if (user == opponent) MovesWithPP({move, 10}, {MOVE_SPLASH, 10});
        }
    } WHEN {
        for (u32 i = 0; i < 6; i++)
            TURN { MOVE(user, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(user, move); }
    } SCENE {
        for (u32 i = 0; i < 7; i++)
            ANIMATION(ANIM_TYPE_MOVE, move, user);
    }
}
