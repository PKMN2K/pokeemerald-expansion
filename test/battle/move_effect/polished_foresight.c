#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Foresight leaves optional Miracle Eye hidden and evasion rules native")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 hidden = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 hiding = 0; hiding < 2; hiding++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; hidden = hiding; }
    if (!hidden)
        PASSES_RANDOMLY(50, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(hidden ? ABILITY_NO_GUARD : ABILITY_NONE); Speed(user == player ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(hidden ? ABILITY_NO_GUARD : ABILITY_NONE); Speed(user == opponent ? 50 : 100); }
    } WHEN {
        if (!hidden)
            TURN { MOVE(user, MOVE_SWEET_SCENT, hit: TRUE); }
        TURN { MOVE(user, MOVE_MIRACLE_EYE); if (hidden) MOVE(target, MOVE_FLY); }
        if (!hidden)
            TURN { MOVE(user, MOVE_ZAP_CANNON); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MIRACLE_EYE, user);
        if (!hidden)
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_ZAP_CANNON, user);
            HP_BAR(target);
        }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.miracleEye, TRUE);
        EXPECT_EQ((u32)target->volatiles.foresight, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Foresight cannot identify a hidden target through No Guard or Lock-On")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move hidden = MOVE_FLY;
    u32 bypass = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 underground = 0; underground < 2; underground++)
            for (u32 kind = 0; kind < 4; kind++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; hidden = underground ? MOVE_DIG : MOVE_FLY; bypass = kind; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability((user == player && bypass == 1) || (target == player && bypass == 2) ? ABILITY_NO_GUARD : ABILITY_NONE);
            Speed(user == player ? 50 : 100); MovesWithPP({MOVE_FORESIGHT, 1}, {MOVE_LOCK_ON, 10}, {MOVE_CELEBRATE, 10}, {hidden, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability((user == opponent && bypass == 1) || (target == opponent && bypass == 2) ? ABILITY_NO_GUARD : ABILITY_NONE);
            Speed(user == opponent ? 50 : 100); MovesWithPP({MOVE_FORESIGHT, 1}, {MOVE_LOCK_ON, 10}, {MOVE_CELEBRATE, 10}, {hidden, 10});
        }
    } WHEN {
        if (bypass == 3)
            TURN { MOVE(user, MOVE_LOCK_ON); }
        TURN { MOVE(target, hidden); MOVE(user, MOVE_FORESIGHT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, hidden, target);
        if (B_POLISHED_MOVE_BALANCE || bypass == 0)
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
        else
            ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.foresight, !B_POLISHED_MOVE_BALANCE && bypass != 0);
        EXPECT_EQ(user->pp[0], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Foresight removes only Ghost immunity for Normal and Fighting hits")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move attack = MOVE_TACKLE;
    enum Species species = SPECIES_GASTLY;
    bool32 removes = TRUE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 4; kind++)
            PARAMETRIZE {
                user = side ? opponent : player; target = side ? player : opponent;
                attack = kind == 0 ? MOVE_TACKLE : kind == 1 ? MOVE_LOW_KICK : kind == 2 ? MOVE_PSYCHIC : MOVE_THUNDER_SHOCK;
                species = kind < 2 ? SPECIES_GASTLY : kind == 2 ? SPECIES_UMBREON : SPECIES_RHYHORN;
                removes = kind < 2;
            }
    GIVEN {
        PLAYER(user == player ? SPECIES_WOBBUFFET : species) { Ability(ABILITY_NONE); Speed(100); MaxHP(1000); HP(1000); }
        OPPONENT(user == opponent ? SPECIES_WOBBUFFET : species) { Ability(ABILITY_NONE); Speed(50); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(user, attack); }
        TURN { MOVE(user, MOVE_FORESIGHT); }
        TURN { MOVE(user, attack); }
    } SCENE {
        NOT HP_BAR(target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
        if (removes)
        {
            ANIMATION(ANIM_TYPE_MOVE, attack, user);
            HP_BAR(target);
        }
        else
            NOT HP_BAR(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.foresight, TRUE);
        EXPECT_EQ(target->hp < target->maxHP, removes);
    }
}

SINGLE_BATTLE_TEST("Polished Foresight bypasses Substitute but respects Protect and consumes PP")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move setup = MOVE_CELEBRATE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; setup = kind == 0 ? MOVE_CELEBRATE : kind == 1 ? MOVE_SUBSTITUTE : MOVE_PROTECT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == player ? 50 : 100); if (user == player) MovesWithPP({MOVE_FORESIGHT, 1}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == opponent ? 50 : 100); if (user == opponent) MovesWithPP({MOVE_FORESIGHT, 1}); }
    } WHEN {
        TURN { MOVE(target, setup); MOVE(user, MOVE_FORESIGHT); }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.foresight, setup != MOVE_PROTECT);
        EXPECT_EQ(user->pp[0], 0);
        if (setup == MOVE_SUBSTITUTE)
            EXPECT_GT((u32)target->volatiles.substituteHP, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Foresight repeat fails and ordinary switching clears identification")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); MovesWithPP({MOVE_FORESIGHT, 3}, {MOVE_CELEBRATE, 10}); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MovesWithPP({MOVE_FORESIGHT, 3}, {MOVE_CELEBRATE, 10}); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(user, MOVE_FORESIGHT); }
        TURN { MOVE(user, MOVE_FORESIGHT); }
        TURN { SWITCH(target, 1); }
        TURN { SWITCH(target, 0); }
        TURN { MOVE(user, MOVE_FORESIGHT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
        MESSAGE("But it failed!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.foresight, TRUE);
        EXPECT_EQ(user->pp[0], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Foresight No Guard and Mold Breaker still bypass visible Wonder Skin")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NO_GUARD;
    for (u32 side = 0; side < 2; side++)
        for (u32 mold = 0; mold < 2; mold++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ability = mold ? ABILITY_MOLD_BREAKER : ABILITY_NO_GUARD; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ability : ABILITY_WONDER_SKIN); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ability : ABILITY_WONDER_SKIN); Speed(50); }
    } WHEN {
        TURN { MOVE(user, MOVE_FORESIGHT, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.foresight, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Foresight retains native identification rules during active Terastal")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 hidden = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 hiding = 0; hiding < 2; hiding++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; hidden = hiding; }
    if (!hidden)
        PASSES_RANDOMLY(50, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(hidden ? ABILITY_NO_GUARD : ABILITY_NONE); Speed(user == player ? 50 : 100); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(hidden ? ABILITY_NO_GUARD : ABILITY_NONE); Speed(user == opponent ? 50 : 100); TeraType(TYPE_NORMAL); }
    } WHEN {
        if (!hidden)
            TURN { MOVE(user, MOVE_SWEET_SCENT, hit: TRUE); }
        TURN { MOVE(user, MOVE_FORESIGHT, gimmick: GIMMICK_TERA); if (hidden) MOVE(target, MOVE_FLY); }
        if (!hidden)
            TURN { MOVE(user, MOVE_ZAP_CANNON); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
        if (!hidden)
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_ZAP_CANNON, user);
            HP_BAR(target);
        }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.foresight, TRUE);
    }
}

DOUBLE_BATTLE_TEST("Polished Foresight retains native No Guard behavior in optional doubles")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    PARAMETRIZE { user = playerLeft; target = opponentLeft; }
    PARAMETRIZE { user = opponentLeft; target = playerLeft; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NO_GUARD); Speed(user == playerLeft ? 50 : 100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NO_GUARD); Speed(user == opponentLeft ? 50 : 100); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(target, MOVE_DIG, target: user); MOVE(user, MOVE_FORESIGHT, target: target); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.foresight, TRUE);
    }
}

SINGLE_BATTLE_TEST("Polished Foresight uses its raw source accuracy under stage penalties and Wonder Skin")
{
    struct BattlePokemon *user = player, *target = opponent;
    s32 stage = -6;
    bool32 wonder = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (s32 delta = -6; delta <= 0; delta++)
            if (delta == -6 || delta == -5 || delta == -4 || delta == 0)
                for (u32 skin = 0; skin < 2; skin++)
                    PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; stage = delta; wonder = skin; }
    u32 chance = B_POLISHED_MOVE_BALANCE ? min(100, (wonder ? 50 : 255) * 3 / (3 - stage)) : 100;
    if (chance < 100)
        PASSES_RANDOMLY(chance, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(target == player && wonder ? ABILITY_WONDER_SKIN : ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(target == opponent && wonder ? ABILITY_WONDER_SKIN : ABILITY_NONE); Speed(50); }
    } WHEN {
        for (u32 i = 0; i < -stage; i++)
            TURN { MOVE(target, MOVE_SAND_ATTACK, hit: TRUE); }
        TURN { MOVE(user, MOVE_FORESIGHT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
    }
}

SINGLE_BATTLE_TEST("Polished Foresight retains negative evasion when calculating following hits")
{
    struct BattlePokemon *user = player, *target = opponent;
    s32 evasion = -2, accuracy = 0;
    for (u32 side = 0; side < 2; side++)
        for (s32 eva = -6; eva <= 6; eva += 2)
            for (u32 reduced = 0; reduced < 2; reduced++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; evasion = eva; accuracy = reduced ? -2 : 0; }
    s32 combined = accuracy - (B_POLISHED_MOVE_BALANCE ? min(0, evasion) : 0);
    u32 chance = min(100, combined >= 0 ? 50 * (3 + combined) / 3 : 50 * 3 / (3 - combined));
    if (chance < 100)
        PASSES_RANDOMLY(chance, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        for (u32 i = 0; i < (evasion < 0 ? -evasion / 2 : evasion); i++)
        {
            if (evasion < 0)
                TURN { MOVE(user, MOVE_SWEET_SCENT, hit: TRUE); }
            else
                TURN { MOVE(target, MOVE_DOUBLE_TEAM); }
        }
        for (u32 i = 0; i < -accuracy; i++)
            TURN { MOVE(target, MOVE_SAND_ATTACK, hit: TRUE); }
        TURN { MOVE(user, MOVE_FORESIGHT, hit: TRUE); }
        TURN { MOVE(user, MOVE_ZAP_CANNON); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FORESIGHT, user);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ZAP_CANNON, user);
        HP_BAR(target);
    }
}
