#include "global.h"
#include "test/battle.h"

static const enum Move sSelfThawMoves[] = {MOVE_SACRED_FIRE, MOVE_SCALD, MOVE_FLARE_BLITZ};

SINGLE_BATTLE_TEST("Polished self-thaw precedes confusion whether the user attacks or hits itself")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    bool32 selfHit = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 blocked = 0; blocked < 2; blocked++)
                PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; selfHit = blocked; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_NONE : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_NONE : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_CONFUSE_RAY); MOVE(holder, move, hit: TRUE, secondaryEffect: FALSE, WITH_RNG(RNG_CONFUSION, selfHit)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, other);
        if (B_POLISHED_MOVE_BALANCE) STATUS_ICON(holder, none: TRUE);
        ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, holder);
        if (selfHit) {
            HP_BAR(holder);
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, holder); STATUS_ICON(holder, none: TRUE); }
        } else {
            if (!B_POLISHED_MOVE_BALANCE) STATUS_ICON(holder, none: TRUE);
            ANIMATION(ANIM_TYPE_MOVE, move, holder);
            NOT STATUS_ICON(holder, none: TRUE);
        }
    } THEN {
        EXPECT_EQ(holder->status1, selfHit && !B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], selfHit ? 5 : 4);
        if (selfHit) EXPECT_LT(holder->hp, 1000);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw precedes infatuation whether the user acts or is immobilized")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    bool32 blocked = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 stop = 0; stop < 2; stop++)
                PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; blocked = stop; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_NONE : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_NONE : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_ATTRACT); MOVE(holder, move, hit: TRUE, secondaryEffect: FALSE, WITH_RNG(RNG_INFATUATION, blocked)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ATTRACT, other);
        if (B_POLISHED_MOVE_BALANCE) STATUS_ICON(holder, none: TRUE);
        ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_INFATUATION, holder);
        if (blocked) {
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, move, holder); STATUS_ICON(holder, none: TRUE); }
        } else {
            if (!B_POLISHED_MOVE_BALANCE) STATUS_ICON(holder, none: TRUE);
            ANIMATION(ANIM_TYPE_MOVE, move, holder);
            NOT STATUS_ICON(holder, none: TRUE);
        }
    } THEN {
        EXPECT_EQ(holder->status1, blocked && !B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], blocked ? 5 : 4);
        if (blocked) EXPECT_EQ(holder->hp, 1000);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw clears freeze before a newly applied Disable stops the chosen move")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    bool32 disable = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 stop = 0; stop < 2; stop++)
                PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; disable = stop; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 150 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_NONE : ABILITY_PRANKSTER); if (holder == player) { Status1(STATUS1_NONE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 150 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_NONE : ABILITY_PRANKSTER); if (holder == opponent) { Status1(STATUS1_NONE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
    } WHEN {
        TURN { MOVE(holder, move, hit: TRUE, secondaryEffect: FALSE); MOVE(other, MOVE_ICE_BEAM, secondaryEffect: TRUE); }
        // Prankster applies Disable after legal selection but before the action.
        TURN { MOVE(other, disable ? MOVE_DISABLE : MOVE_CELEBRATE); MOVE(holder, move, hit: TRUE, secondaryEffect: FALSE); }
    } SCENE {
        STATUS_ICON(holder, freeze: TRUE);
        if (disable) ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, other);
        if (B_POLISHED_MOVE_BALANCE || !disable) STATUS_ICON(holder, none: TRUE);
        if (disable) NOT ANIMATION(ANIM_TYPE_MOVE, move, holder);
        else ANIMATION(ANIM_TYPE_MOVE, move, holder);
    } THEN {
        EXPECT_EQ(holder->status1, disable && !B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], disable ? 4 : 3);
        EXPECT_EQ((u32)holder->volatiles.disabledMove, disable ? move : MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw remains after effective flinch and respects Inner Focus and Steadfast")
{
    static const enum Ability abilities[] = {ABILITY_NONE, ABILITY_INNER_FOCUS, ABILITY_STEADFAST};
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    enum Ability ability = ABILITY_NONE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 a = 0; a < ARRAY_COUNT(abilities); a++)
                PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; ability = abilities[a]; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ability : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ability : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_BITE, secondaryEffect: TRUE); MOVE(holder, move, hit: TRUE, secondaryEffect: FALSE); }
    } THEN {
        EXPECT_EQ(holder->status1, ability == ABILITY_INNER_FOCUS ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], ability == ABILITY_INNER_FOCUS ? 4 : 5);
        EXPECT_EQ(holder->statStages[STAT_SPEED], ability == ABILITY_STEADFAST ? DEFAULT_STAT_STAGE + 1 : DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw does not clear freeze when flinch stops a confused user")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_NONE : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_NONE : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_CONFUSE_RAY); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
        TURN { MOVE(other, MOVE_BITE, secondaryEffect: TRUE); MOVE(holder, move, WITH_RNG(RNG_CONFUSION, TRUE)); }
    } SCENE {
        NONE_OF { STATUS_ICON(holder, none: TRUE); ANIMATION(ANIM_TYPE_MOVE, move, holder); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw leaves optional thaw moves at their native stage")
{
    static const enum Move moves[] = {MOVE_FLAME_WHEEL, MOVE_STEAM_ERUPTION};
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_FLAME_WHEEL;
    bool32 attract = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 block = 0; block < 2; block++)
                PARAMETRIZE { move = moves[m]; holder = side ? opponent : player; other = side ? player : opponent; attract = block; }
    GIVEN {
        ASSUME(MoveThawsUser(move));
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_NONE : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_NONE : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
    } WHEN {
        TURN { MOVE(other, attract ? MOVE_ATTRACT : MOVE_CONFUSE_RAY); MOVE(holder, move, WITH_RNG(attract ? RNG_INFATUATION : RNG_CONFUSION, TRUE)); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
    }
}

DOUBLE_BATTLE_TEST("Polished self-thaw leaves doubles ordering native")
{
    struct BattlePokemon *holder = playerLeft, *other = opponentLeft;
    enum Move move = MOVE_SACRED_FIRE;
    bool32 attract = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 block = 0; block < 2; block++)
                PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponentLeft : playerLeft; other = side ? playerLeft : opponentLeft; attract = block; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == playerLeft ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); if (holder == playerLeft) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}); } }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponentLeft ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); if (holder == opponentLeft) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}); } }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, attract ? MOVE_ATTRACT : MOVE_CONFUSE_RAY, target: holder); MOVE(holder, move, target: other, WITH_RNG(attract ? RNG_INFATUATION : RNG_CONFUSION, TRUE)); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw leaves either active Terastal native")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    bool32 attract = FALSE, holderTera = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 block = 0; block < 2; block++)
                for (u32 tera = 0; tera < 2; tera++)
                    PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; attract = block; holderTera = tera; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_NONE : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } TeraType(TYPE_NORMAL); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_NONE : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } TeraType(TYPE_NORMAL); }
    } WHEN {
        TURN { MOVE(other, attract ? MOVE_ATTRACT : MOVE_CONFUSE_RAY, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA); MOVE(holder, move, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE, WITH_RNG(attract ? RNG_INFATUATION : RNG_CONFUSION, TRUE)); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw uses source move identity even when Normalize changes its type")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_NORMALIZE : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_NORMALIZE : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_CONFUSE_RAY); MOVE(holder, move, WITH_RNG(RNG_CONFUSION, TRUE)); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw does not exempt ordinary Fire or Water attacks from freezing")
{
    static const enum Move moves[] = {MOVE_FIRE_PUNCH, MOVE_EMBER, MOVE_WATER_GUN};
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_FIRE_PUNCH;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = moves[m]; holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        ASSUME(!MoveThawsUser(move));
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_NONE : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_NONE : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_CONFUSE_RAY); MOVE(holder, move, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? 255 : FALSE)); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ(holder->hp, 1000);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw leaves optional Frostbite ordering native")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); Gender(MON_FEMALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == player ? ABILITY_NONE : ABILITY_NONE); if (holder == player) { Status1(STATUS1_FROSTBITE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); Gender(MON_MALE); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(holder == opponent ? ABILITY_NONE : ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FROSTBITE); MovesWithPP({move, 5}, {MOVE_TACKLE, 5}, {MOVE_CELEBRATE, 10}); } }
    } WHEN {
        TURN { MOVE(other, MOVE_CONFUSE_RAY); MOVE(holder, move, WITH_RNG(RNG_CONFUSION, TRUE)); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_FROSTBITE);
        EXPECT_EQ(holder->pp[0], 5);
    }
}

SINGLE_BATTLE_TEST("Polished self-thaw persists after a blocked move and switching out and back")
{
    struct BattlePokemon *holder = player, *other = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    for (u32 m = 0; m < ARRAY_COUNT(sSelfThawMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSelfThawMoves[m]; holder = side ? opponent : player; other = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); if (holder == player) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}); } }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); Ability(ABILITY_NONE); if (holder == opponent) { Status1(STATUS1_FREEZE); MovesWithPP({move, 5}); } }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(other, MOVE_CONFUSE_RAY); MOVE(holder, move, WITH_RNG(RNG_CONFUSION, TRUE)); }
        TURN { SWITCH(holder, 1); }
        TURN { SWITCH(holder, 0); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 5);
        EXPECT_EQ((u32)holder->volatiles.confusionTimer, 0);
    }
}
