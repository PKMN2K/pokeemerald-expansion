#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Protect interruption preserves the source consecutive probability")
{
    struct BattlePokemon *user = player;
    enum Move move = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; move = kind ? MOVE_ENDURE : MOVE_PROTECT; }
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE ? 3 : 1, RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { if (user == player) { Status1(STATUS1_PARALYSIS); MovesWithPP({move, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { if (user == opponent) { Status1(STATUS1_PARALYSIS); MovesWithPP({move, 10}); } }
    } WHEN {
        TURN { MOVE(user, move, WITH_RNG(RNG_PARALYSIS, FALSE)); }
        TURN { MOVE(user, move, WITH_RNG(RNG_PARALYSIS, TRUE)); }
        TURN { MOVE(user, move, WITH_RNG(RNG_PARALYSIS, FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, user);
        ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN { EXPECT_EQ(user->pp[0], 8); }
}

SINGLE_BATTLE_TEST("Polished Protect counter resets when another selected move cannot execute")
{
    enum Move move = MOVE_PROTECT;
    PARAMETRIZE { move = MOVE_PROTECT; }
    PARAMETRIZE { move = MOVE_ENDURE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_PARALYSIS); MovesWithPP({move, 10}, {MOVE_SPLASH, 10}); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, move, WITH_RNG(RNG_PARALYSIS, FALSE)); }
        TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_PARALYSIS, TRUE)); }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.consecutiveMoveUses, B_POLISHED_MOVE_BALANCE ? 0 : 1);
        EXPECT_EQ(player->pp[1], 10);
    }
}

SINGLE_BATTLE_TEST("Polished Protect counter resets on a player item action")
{
    enum Move move = MOVE_PROTECT;
    PARAMETRIZE { move = MOVE_PROTECT; }
    PARAMETRIZE { move = MOVE_ENDURE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(50); MaxHP(200); MovesWithPP({move, 10}); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, move); }
        TURN { USE_ITEM(player, ITEM_POTION, partyIndex: 0); }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.consecutiveMoveUses, B_POLISHED_MOVE_BALANCE ? 0 : 1);
        EXPECT_EQ(player->pp[0], 9);
        EXPECT_GT(player->hp, 50);
    }
}

SINGLE_BATTLE_TEST("Polished Protect and Endure called by Sleep Talk reset at each selection")
{
    enum Move move = MOVE_PROTECT;
    PARAMETRIZE { move = MOVE_PROTECT; }
    PARAMETRIZE { move = MOVE_ENDURE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Status1(STATUS1_SLEEP_TURN(4)); MovesWithPP({MOVE_SLEEP_TALK, 10}, {move, 0}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SLEEP_TALK); }
        TURN { MOVE(player, MOVE_SLEEP_TALK, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        ANIMATION(ANIM_TYPE_MOVE, move, player);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.consecutiveMoveUses, B_POLISHED_MOVE_BALANCE ? 1 : 2);
        EXPECT_EQ(player->pp[0], 8);
        EXPECT_EQ(player->pp[1], 0);
    }
}

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

DOUBLE_BATTLE_TEST("Polished Protect optional doubles retain native consecutive odds and count")
{
    enum Move move = MOVE_PROTECT;
    PARAMETRIZE { move = MOVE_PROTECT; }
    PARAMETRIZE { move = MOVE_ENDURE; }
    PASSES_RANDOMLY(1, ProtectDenominator(6, FALSE), RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MovesWithPP({move, 10}); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        for (u32 n = 0; n < 6; n++)
            TURN { MOVE(playerLeft, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(playerLeft, move); }
    } SCENE {
        for (u32 n = 0; n < 7; n++)
            ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
    } THEN {
        EXPECT_EQ((u32)playerLeft->volatiles.consecutiveMoveUses, 7);
        EXPECT_EQ(playerLeft->pp[0], 3);
    }
}

SINGLE_BATTLE_TEST("Polished Protect either active gimmick retains native consecutive odds and count")
{
    enum Move move = MOVE_PROTECT;
    bool32 userTera = FALSE;
    for (u32 kind = 0; kind < 2; kind++)
        for (u32 tera = 0; tera < 2; tera++)
            PARAMETRIZE { move = kind ? MOVE_ENDURE : MOVE_PROTECT; userTera = tera; }
    PASSES_RANDOMLY(1, ProtectDenominator(6, FALSE), RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { TeraType(TYPE_PSYCHIC); MovesWithPP({move, 10}, {MOVE_SPLASH, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { TeraType(TYPE_PSYCHIC); }
    } WHEN {
        TURN {
            MOVE(player, MOVE_SPLASH, gimmick: userTera ? GIMMICK_TERA : GIMMICK_NONE);
            MOVE(opponent, MOVE_SPLASH, gimmick: userTera ? GIMMICK_NONE : GIMMICK_TERA);
        }
        for (u32 n = 0; n < 6; n++)
            TURN { MOVE(player, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(player, move); }
    } SCENE {
        for (u32 n = 0; n < 7; n++)
            ANIMATION(ANIM_TYPE_MOVE, move, player);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.consecutiveMoveUses, 7);
        EXPECT_EQ(player->pp[0], 3);
    }
}

WILD_BATTLE_TEST("Polished Protect disobedient alternate move preserves the selected protection chain")
{
    enum Move move = MOVE_PROTECT;
    u32 otId = 1;
    PARAMETRIZE { move = MOVE_PROTECT; }
    PARAMETRIZE { move = MOVE_ENDURE; }
    PASSES_RANDOMLY(1, B_POLISHED_MOVE_BALANCE ? 3 : ProtectDenominator(2, FALSE), RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MovesWithPP({move, 5}, {MOVE_SPLASH, 5}); }
        if (B_POLISHED_MOVE_BALANCE) SetMonData(&PLAYER_PARTY[0], MON_DATA_OT_ID, &otId);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, move, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 0)); }
        if (B_POLISHED_MOVE_BALANCE)
            TURN { MOVE(player, move, WITH_RNG(RNG_POLISHED_OBEDIENCE_SECOND, 0)); }
        else TURN { MOVE(player, move, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(player, move, WITH_RNG(RNG_POLISHED_OBEDIENCE_FIRST, 0)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        ANIMATION(ANIM_TYPE_MOVE, B_POLISHED_MOVE_BALANCE ? MOVE_SPLASH : move, player);
        ANIMATION(ANIM_TYPE_MOVE, move, player);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.consecutiveMoveUses, B_POLISHED_MOVE_BALANCE ? 2 : 3);
        EXPECT_EQ(player->pp[0], B_POLISHED_MOVE_BALANCE ? 3 : 2);
        EXPECT_EQ(player->pp[1], B_POLISHED_MOVE_BALANCE ? 4 : 5);
    }
}
