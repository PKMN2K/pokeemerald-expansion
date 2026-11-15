#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Roar priority matches source Prankster Dark and Soundproof gates")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Ability attack = ABILITY_NONE, defense = ABILITY_NONE;
    bool32 dark = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 ability = 0; ability < 3; ability++)
            for (u32 immunity = 0; immunity < 4; immunity++)
                for (u32 type = 0; type < 2; type++)
                    PARAMETRIZE {
                        user = side ? opponent : player; target = side ? player : opponent;
                        attack = ability == 0 ? ABILITY_NONE : ability == 1 ? ABILITY_PRANKSTER : ABILITY_MOLD_BREAKER;
                        defense = immunity == 0 ? ABILITY_NONE : immunity == 1 ? ABILITY_SOUNDPROOF : immunity == 2 ? ABILITY_SUCTION_CUPS : ABILITY_ARMOR_TAIL;
                        dark = type;
                    }
    bool32 darkBlocks = B_POLISHED_MOVE_BALANCE && dark && attack == ABILITY_PRANKSTER;
    bool32 soundBlocks = defense == ABILITY_SOUNDPROOF && attack != ABILITY_MOLD_BREAKER
        && !(B_POLISHED_MOVE_BALANCE && attack == ABILITY_PRANKSTER);
    bool32 suctionBlocks = defense == ABILITY_SUCTION_CUPS && attack != ABILITY_MOLD_BREAKER;
    bool32 succeeds = !darkBlocks && !soundBlocks && !suctionBlocks;
    GIVEN {
        ASSUME(GetMovePriority(MOVE_ROAR) == -6);
        PLAYER(target == player && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == player ? attack : defense); Speed(100); MovesWithPP({MOVE_ROAR, 2}); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(target == opponent && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == opponent ? attack : defense); Speed(50); MovesWithPP({MOVE_ROAR, 2}); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR); }
    } SCENE {
        if (succeeds) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
    } THEN {
        EXPECT_EQ(target->species, succeeds ? SPECIES_WYNAUT : dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET);
        EXPECT_EQ(user->pp[0], 1);
    }
}

SINGLE_BATTLE_TEST("Polished Roar priority lets Prankster bypass non-Dark Soundproof")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_PRANKSTER : ABILITY_SOUNDPROOF); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_SOUNDPROOF); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else ABILITY_POPUP(target, ABILITY_SOUNDPROOF);
    } THEN {
        EXPECT_EQ(target->species, B_POLISHED_MOVE_BALANCE ? SPECIES_WYNAUT : SPECIES_WOBBUFFET);
    }
}

SINGLE_BATTLE_TEST("Polished Roar priority checks hidden Dark targets reached with No Guard")
{
    struct BattlePokemon *user = player, *target = opponent;
    enum Move hidden = MOVE_FLY;
    for (u32 side = 0; side < 2; side++)
        for (u32 kind = 0; kind < 2; kind++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; hidden = kind ? MOVE_DIG : MOVE_FLY; }
    GIVEN {
        PLAYER(target == player ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_PRANKSTER : ABILITY_NO_GUARD); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(target == opponent ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_NO_GUARD); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(target, hidden); MOVE(user, MOVE_ROAR); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, hidden, target);
        if (B_POLISHED_MOVE_BALANCE) NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
    } THEN {
        EXPECT_EQ(target->species, B_POLISHED_MOVE_BALANCE ? SPECIES_UMBREON : SPECIES_WYNAUT);
    }
}

SINGLE_BATTLE_TEST("Polished Roar priority retains earlier Protect and independent Substitute rules")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 dark = FALSE, protect = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 type = 0; type < 2; type++)
            for (u32 shield = 0; shield < 2; shield++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; dark = type; protect = shield; }
    bool32 succeeds = !B_POLISHED_MOVE_BALANCE || (!dark && !protect);
    GIVEN {
        PLAYER(target == player && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_PRANKSTER : ABILITY_NONE); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(target == opponent && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(target, protect ? MOVE_PROTECT : MOVE_SUBSTITUTE); MOVE(user, MOVE_ROAR); }
    } SCENE {
        if (succeeds) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
    } THEN {
        EXPECT_EQ(target->species, succeeds ? SPECIES_WYNAUT : dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET);
    }
}

SINGLE_BATTLE_TEST("Polished Roar priority uses the reflector ability when returning Roar to a Dark user")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(user == player ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(user == opponent ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR); }
    } SCENE {
        ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, target);
    } THEN {
        EXPECT_EQ(user->species, SPECIES_WYNAUT);
        EXPECT_EQ(target->species, SPECIES_WOBBUFFET);
    }
}

SINGLE_BATTLE_TEST("Polished Roar priority leaves optional Whirlwind and active gimmicks native")
{
    struct BattlePokemon *user = player, *target = opponent;
    u32 kind = 0;
    bool32 dark = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 option = 0; option < 3; option++)
            for (u32 type = 0; type < 2; type++)
                PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; kind = option; dark = type; }
    enum Move move = kind ? MOVE_ROAR : MOVE_WHIRLWIND;
    bool32 succeeds = dark || kind == 0;
    GIVEN {
        PLAYER(target == player && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_PRANKSTER : dark ? ABILITY_NONE : ABILITY_SOUNDPROOF); Speed(100); TeraType(target == player && dark ? TYPE_DARK : TYPE_NORMAL); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(target == opponent && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_PRANKSTER : dark ? ABILITY_NONE : ABILITY_SOUNDPROOF); Speed(50); TeraType(target == opponent && dark ? TYPE_DARK : TYPE_NORMAL); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        if (kind == 2) TURN { MOVE(target, MOVE_CELEBRATE, gimmick: GIMMICK_TERA); }
        TURN { MOVE(user, move, gimmick: kind == 1 ? GIMMICK_TERA : GIMMICK_NONE); }
    } SCENE {
        if (succeeds) ANIMATION(ANIM_TYPE_MOVE, move, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, move, user);
    } THEN {
        EXPECT_EQ(target->species, succeeds ? SPECIES_WYNAUT : SPECIES_WOBBUFFET);
    }
}

SINGLE_BATTLE_TEST("Polished Roar priority leaves dynamically retyped Roar native")
{
    struct BattlePokemon *user = player, *target = opponent;
    PARAMETRIZE { user = player; target = opponent; }
    PARAMETRIZE { user = opponent; target = player; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_PRANKSTER : ABILITY_SOUNDPROOF); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_SOUNDPROOF); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(target, MOVE_ELECTRIFY); MOVE(user, MOVE_ROAR); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ELECTRIFY, target);
        ABILITY_POPUP(target, ABILITY_SOUNDPROOF);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
    } THEN {
        EXPECT_EQ(target->species, SPECIES_WOBBUFFET);
    }
}

DOUBLE_BATTLE_TEST("Polished Roar priority leaves optional doubles native")
{
    struct BattlePokemon *user = playerLeft, *target = opponentLeft;
    bool32 dark = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 type = 0; type < 2; type++)
            PARAMETRIZE { user = side ? opponentLeft : playerLeft; target = side ? playerLeft : opponentLeft; dark = type; }
    GIVEN {
        PLAYER(target == playerLeft && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == playerLeft ? ABILITY_PRANKSTER : dark ? ABILITY_NONE : ABILITY_SOUNDPROOF); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        PLAYER(SPECIES_RATTATA) { Ability(ABILITY_NONE); }
        OPPONENT(target == opponentLeft && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == opponentLeft ? ABILITY_PRANKSTER : dark ? ABILITY_NONE : ABILITY_SOUNDPROOF); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_RATTATA) { Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR, target: target); }
    } SCENE {
        if (dark) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
    } THEN {
        EXPECT_EQ(target->species, dark ? SPECIES_RATTATA : SPECIES_WOBBUFFET);
    }
}

WILD_BATTLE_TEST("Polished Roar priority applies source gates before ending wild battles")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 dark = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 type = 0; type < 2; type++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; dark = type; }
    bool32 succeeds = B_POLISHED_MOVE_BALANCE ? !dark : dark;
    GIVEN {
        PLAYER(target == player && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Ability(user == player ? ABILITY_PRANKSTER : dark ? ABILITY_NONE : ABILITY_SOUNDPROOF); Speed(100); }
        OPPONENT(target == opponent && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Ability(user == opponent ? ABILITY_PRANKSTER : dark ? ABILITY_NONE : ABILITY_SOUNDPROOF); Speed(50); Moves(user == opponent ? MOVE_ROAR : MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR); }
        if (!succeeds) TURN { MOVE(player, MOVE_TACKLE, hit: TRUE); }
    } SCENE {
        if (succeeds) ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user);
        else { NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, user); ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player); HP_BAR(opponent); }
    } THEN {
        if (succeeds) EXPECT_EQ(gBattleOutcome, user == player ? B_OUTCOME_PLAYER_TELEPORTED : B_OUTCOME_MON_TELEPORTED);
    }
}

SINGLE_BATTLE_TEST("Polished Roar priority rejects Dark targets before Magic Bounce")
{
    struct BattlePokemon *user = player, *target = opponent;
    bool32 dark = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 type = 0; type < 2; type++)
            PARAMETRIZE { user = side ? opponent : player; target = side ? player : opponent; dark = type; }
    bool32 reflected = !B_POLISHED_MOVE_BALANCE || !dark;
    GIVEN {
        PLAYER(target == player && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == player ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(75); }
        OPPONENT(target == opponent && dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET) { Ability(user == opponent ? ABILITY_PRANKSTER : ABILITY_MAGIC_BOUNCE); Speed(50); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(25); }
    } WHEN {
        TURN { MOVE(user, MOVE_ROAR); }
    } SCENE {
        if (reflected) { ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE); ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, target); }
        else NONE_OF { ABILITY_POPUP(target, ABILITY_MAGIC_BOUNCE); ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, target); }
    } THEN {
        EXPECT_EQ(user->species, reflected ? SPECIES_WYNAUT : SPECIES_WOBBUFFET);
        EXPECT_EQ(target->species, dark ? SPECIES_UMBREON : SPECIES_WOBBUFFET);
    }
}
