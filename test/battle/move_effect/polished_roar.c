#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Roar respects Protect even with No Guard and Substitute")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 noGuard = FALSE, sub = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 ability = 0; ability < 2; ability++)
            for (u32 shield = 0; shield < 2; shield++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; noGuard = ability; sub = shield; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(user == player && noGuard ? ABILITY_NO_GUARD : ABILITY_KEEN_EYE); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(user == opponent && noGuard ? ABILITY_NO_GUARD : ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        if (sub) TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(target, MOVE_PROTECT); MOVE(user, MOVE_ROAR); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, target);
        if (B_POLISHED_MOVE_BALANCE) NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
    } THEN {
        EXPECT_EQ(target->species, B_POLISHED_MOVE_BALANCE ? SPECIES_WOBBUFFET : SPECIES_WYNAUT);
        if (sub && B_POLISHED_MOVE_BALANCE) EXPECT_GT((u32)target->volatiles.substituteHP, 0);
    }
}

SINGLE_BATTLE_TEST("Polished Roar bypasses Substitute and accuracy or evasion changes")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 sub = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 shield = 0; shield < 2; shield++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; sub = shield; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_BATTLE_ARMOR); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_BATTLE_ARMOR); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_BATTLE_ARMOR); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_BATTLE_ARMOR); }
    } WHEN {
        TURN { MOVE(target, MOVE_SAND_ATTACK, hit: TRUE); }
        TURN { MOVE(target, MOVE_DOUBLE_TEAM); }
        if (sub) TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_ROAR); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
    } THEN {
        EXPECT_EQ(target->species, SPECIES_WYNAUT);
        EXPECT_EQ((u32)target->volatiles.substituteHP, 0);
        EXPECT_EQ(user->statStages[STAT_ACC], DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Roar requires a living trainer replacement")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 reserve = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 3; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; reserve = kind; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_KEEN_EYE); }
        if (target == player && reserve) PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_KEEN_EYE); if (reserve == 1) HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_KEEN_EYE); }
        if (target == opponent && reserve) OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_KEEN_EYE); if (reserve == 1) HP(0); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR); }
    } SCENE {
        if (reserve == 2) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ(target->species, reserve == 2 ? SPECIES_WYNAUT : SPECIES_WOBBUFFET);
    }
}

SINGLE_BATTLE_TEST("Polished Roar retains Suction Cups and Soundproof with Mold Breaker suppression")
{
    enum Ability defense = ABILITY_SUCTION_CUPS;
    bool32 breaker = FALSE;
    for (u32 kind = 0; kind < 2; kind++)
        for (u32 bypass = 0; bypass < 2; bypass++)
            PARAMETRIZE { defense = kind ? ABILITY_SOUNDPROOF : ABILITY_SUCTION_CUPS; breaker = bypass; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(breaker ? ABILITY_MOLD_BREAKER : ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(defense); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN { MOVE(player, MOVE_ROAR); }
    } SCENE {
        if (breaker) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, player);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, player);
    } THEN {
        EXPECT_EQ(opponent->species, breaker ? SPECIES_WYNAUT : SPECIES_WOBBUFFET);
    }
}

SINGLE_BATTLE_TEST("Polished Roar is stopped before Magic Bounce by Protect")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_MAGIC_BOUNCE); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_MAGIC_BOUNCE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN { MOVE(target, MOVE_PROTECT); MOVE(user, MOVE_ROAR); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) {
            NONE_OF { ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE); ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, target); }
        } else {
            ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, target);
        }
    } THEN {
        EXPECT_EQ(user->species, B_POLISHED_MOVE_BALANCE ? SPECIES_WOBBUFFET : SPECIES_WYNAUT);
        EXPECT_EQ(target->species, SPECIES_WOBBUFFET);
    }
}

SINGLE_BATTLE_TEST("Polished Roar retains optional Whirlwind and active Terastal hit rules")
{
    enum Move move = MOVE_WHIRLWIND;
    bool32 hidden = FALSE, targetTera = FALSE;
    for (u32 kind = 0; kind < 3; kind++)
        for (u32 hide = 0; hide < 2; hide++)
            PARAMETRIZE { move = kind ? MOVE_ROAR : MOVE_WHIRLWIND; targetTera = kind == 2; hidden = hide; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_KEEN_EYE); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_KEEN_EYE); TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        if (hidden) TURN { MOVE(player, MOVE_LOCK_ON); }
        TURN { MOVE(opponent, hidden ? MOVE_DIG : MOVE_PROTECT, gimmick: targetTera ? GIMMICK_TERA : GIMMICK_NONE); MOVE(player, move, gimmick: move == MOVE_ROAR && !targetTera ? GIMMICK_TERA : GIMMICK_NONE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_WYNAUT);
    }
}

DOUBLE_BATTLE_TEST("Polished Roar retains native Protect behavior in optional doubles")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_PROTECT); MOVE(playerLeft, MOVE_ROAR, target: opponentLeft); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, playerLeft);
    } THEN {
        EXPECT_EQ(opponentLeft->species, SPECIES_RATTATA);
    }
}

WILD_BATTLE_TEST("Polished Roar ends ordinary wild battles only at equal or higher user level")
{
    struct BattlePokemon *user = player;
    u32 level = 49;
    bool32 bench = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 delta = 0; delta < 3; delta++)
            for (u32 reserve = 0; reserve < 2; reserve++)
                PARAMETRIZE { user = side ? opponent : player; level = 49 + delta; bench = reserve; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(user == player ? level : 50); Speed(100); Ability(ABILITY_KEEN_EYE); }
        if (bench) PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(user == opponent ? level : 50); Speed(50); HP(720); MaxHP(720); Ability(ABILITY_KEEN_EYE); Moves(user == opponent ? MOVE_ROAR : MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR); }
        if (level < 50) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); }
    } SCENE {
        if (level >= 50) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else { MESSAGE("But it failed!"); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent); }
    } THEN {
        if (level >= 50) EXPECT_EQ(gBattleOutcome, user == player ? B_OUTCOME_PLAYER_TELEPORTED : B_OUTCOME_MON_TELEPORTED);
    }
}

SINGLE_BATTLE_TEST("Polished Roar lets No Guard but not Lock On bypass Fly and Dig")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move hidden = MOVE_FLY;
    u32 bypass = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 underground = 0; underground < 2; underground++)
            for (u32 kind = 0; kind < 5; kind++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; hidden = underground ? MOVE_DIG : MOVE_FLY; bypass = kind; }
    bool32 hits = bypass == 1 || bypass == 2 || bypass == 4 || (bypass == 3 && !B_POLISHED_MOVE_BALANCE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Ability((user == player && (bypass == 1 || bypass == 4)) || (target == player && bypass == 2) ? ABILITY_NO_GUARD : ABILITY_KEEN_EYE); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Ability((user == opponent && (bypass == 1 || bypass == 4)) || (target == opponent && bypass == 2) ? ABILITY_NO_GUARD : ABILITY_KEEN_EYE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_KEEN_EYE); }
    } WHEN {
        if (bypass >= 3) TURN { MOVE(user, MOVE_LOCK_ON); }
        TURN { MOVE(target, hidden); MOVE(user, MOVE_ROAR); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, hidden, target);
        if (hits) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
    } THEN {
        EXPECT_EQ(target->species, hits ? SPECIES_WYNAUT : SPECIES_WOBBUFFET);
    }
}
