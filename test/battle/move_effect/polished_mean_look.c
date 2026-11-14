#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Mean Look bypasses Substitute and Protect and consumes PP")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move setup = MOVE_SUBSTITUTE;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; setup = kind == 0 ? MOVE_SUBSTITUTE : kind == 1 ? MOVE_PROTECT : MOVE_CELEBRATE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == player ? 50 : 100); if (user == player) MovesWithPP({MOVE_MEAN_LOOK, 1}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(user == opponent ? 50 : 100); if (user == opponent) MovesWithPP({MOVE_MEAN_LOOK, 1}); }
    } WHEN {
        TURN { MOVE(target, setup); MOVE(user, MOVE_MEAN_LOOK); }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.escapePrevention, B_POLISHED_MOVE_BALANCE || setup != MOVE_SUBSTITUTE);
        EXPECT_EQ((u32)user->volatiles.escapePrevention, FALSE);
        EXPECT_EQ(user->pp[0], 0);
        if (setup == MOVE_SUBSTITUTE)
            EXPECT_GT((u32)target->volatiles.substituteHP, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Mean Look rejects Fly and Dig even through No Guard or Lock-On")
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
            Speed(user == player ? 50 : 100); MovesWithPP({MOVE_MEAN_LOOK, 1}, {MOVE_LOCK_ON, 10}, {MOVE_CELEBRATE, 10}, {hidden, 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability((user == opponent && bypass == 1) || (target == opponent && bypass == 2) ? ABILITY_NO_GUARD : ABILITY_NONE);
            Speed(user == opponent ? 50 : 100); MovesWithPP({MOVE_MEAN_LOOK, 1}, {MOVE_LOCK_ON, 10}, {MOVE_CELEBRATE, 10}, {hidden, 10});
        }
    } WHEN {
        if (bypass == 3)
            TURN { MOVE(user, MOVE_LOCK_ON); }
        TURN { MOVE(target, hidden); MOVE(user, MOVE_MEAN_LOOK); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, hidden, target);
        if (B_POLISHED_MOVE_BALANCE || bypass == 0)
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_MEAN_LOOK, user);
        else
            ANIMATION(ANIM_TYPE_MOVE, MOVE_MEAN_LOOK, user);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.escapePrevention, !B_POLISHED_MOVE_BALANCE && bypass != 0);
        EXPECT_EQ(user->pp[0], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Mean Look separates reflection from Substitute bypass")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability ability = ABILITY_NONE;
    bool32 substitute = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 sub = 0; sub < 2; sub++)
            for (u32 kind = 0; kind < 3; kind++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; substitute = sub; ability = kind == 0 ? ABILITY_NONE : kind == 1 ? ABILITY_INFILTRATOR : ABILITY_MOLD_BREAKER; }
    bool32 reflected = ability != ABILITY_MOLD_BREAKER && (!B_POLISHED_MOVE_BALANCE || !substitute || ability == ABILITY_INFILTRATOR);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ability : ABILITY_MAGIC_BOUNCE); Speed(user == player ? 50 : 100); if (user == player) MovesWithPP({MOVE_MEAN_LOOK, 1}); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ability : ABILITY_MAGIC_BOUNCE); Speed(user == opponent ? 50 : 100); if (user == opponent) MovesWithPP({MOVE_MEAN_LOOK, 1}); }
    } WHEN {
        TURN { if (substitute) MOVE(target, MOVE_SUBSTITUTE); MOVE(user, MOVE_MEAN_LOOK); }
    } SCENE {
        if (reflected)
            ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        else
            NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.escapePrevention, reflected);
        EXPECT_EQ((u32)target->volatiles.escapePrevention, !reflected && (B_POLISHED_MOVE_BALANCE || !substitute));
        EXPECT_EQ(user->pp[0], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Mean Look reflected effect bypasses user Substitute without bouncing twice")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_MAGIC_BOUNCE); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_MAGIC_BOUNCE); Speed(50); }
    } WHEN {
        TURN { MOVE(user, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_MEAN_LOOK); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        NOT ABILITY_POPUP(user, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.escapePrevention, B_POLISHED_MOVE_BALANCE);
        EXPECT_EQ((u32)target->volatiles.escapePrevention, FALSE);
        EXPECT_GT((u32)user->volatiles.substituteHP, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Mean Look Ghost immunity and repeat failure leave trapping unchanged")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 ghost = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 isGhost = 0; isGhost < 2; isGhost++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; ghost = isGhost; }
    GIVEN {
        PLAYER(target == player && ghost ? SPECIES_GASTLY : SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); if (user == player) MovesWithPP({MOVE_MEAN_LOOK, 2}); }
        OPPONENT(target == opponent && ghost ? SPECIES_GASTLY : SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); if (user == opponent) MovesWithPP({MOVE_MEAN_LOOK, 2}); }
    } WHEN {
        TURN { MOVE(user, MOVE_MEAN_LOOK); }
        TURN { MOVE(user, MOVE_MEAN_LOOK); }
    } SCENE {
        if (!ghost)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_MEAN_LOOK, user);
        MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ((u32)target->volatiles.escapePrevention, !ghost);
        EXPECT_EQ(user->pp[0], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Mean Look ordinary trapper switching releases the target")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(user, MOVE_MEAN_LOOK); }
        TURN { SWITCH(user, 1); }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.escapePrevention, FALSE);
    }
}

SINGLE_BATTLE_TEST("Polished Mean Look retains native gates during active Terastal and for optional Spider Web")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 tera = FALSE, hidden = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 gimmick = 0; gimmick < 2; gimmick++)
            for (u32 hide = 0; hide < 2; hide++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; tera = gimmick; hidden = hide; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NO_GUARD); Speed(user == player ? 50 : 100); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NO_GUARD); Speed(user == opponent ? 50 : 100); TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(target, hidden ? MOVE_DIG : MOVE_SUBSTITUTE); MOVE(user, tera ? MOVE_MEAN_LOOK : MOVE_SPIDER_WEB, gimmick: tera ? GIMMICK_TERA : GIMMICK_NONE); }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.escapePrevention, hidden);
    }
}

DOUBLE_BATTLE_TEST("Polished Mean Look retains native gates in optional doubles")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    bool32 hidden = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 hide = 0; hide < 2; hide++)
            PARAMETRIZE { user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; hidden = hide; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NO_GUARD); Speed(user == playerLeft ? 50 : 100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NO_GUARD); Speed(user == opponentLeft ? 50 : 100); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { if (hidden) MOVE(target, MOVE_DIG, target: user); else MOVE(target, MOVE_SUBSTITUTE); MOVE(user, MOVE_MEAN_LOOK, target: target); }
    } THEN {
        EXPECT_EQ((u32)target->volatiles.escapePrevention, hidden);
    }
}

SINGLE_BATTLE_TEST("Polished Mean Look reflects through Protect but never from Fly or Dig")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move setup = MOVE_PROTECT;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; setup = kind == 0 ? MOVE_PROTECT : kind == 1 ? MOVE_FLY : MOVE_DIG; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_NO_GUARD : ABILITY_MAGIC_BOUNCE); Speed(user == player ? 50 : 100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_NO_GUARD : ABILITY_MAGIC_BOUNCE); Speed(user == opponent ? 50 : 100); }
    } WHEN {
        TURN { MOVE(target, setup); MOVE(user, MOVE_MEAN_LOOK); }
    } SCENE {
        if (setup == MOVE_PROTECT)
            ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        else
            NOT ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
    } THEN {
        EXPECT_EQ((u32)user->volatiles.escapePrevention, setup == MOVE_PROTECT);
        EXPECT_EQ((u32)target->volatiles.escapePrevention, !B_POLISHED_MOVE_BALANCE && setup != MOVE_PROTECT);
    }
}
