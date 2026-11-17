#include "global.h"
#include "test/battle.h"

static const enum Move sFreezeMoves[] = {MOVE_ICE_PUNCH, MOVE_ICE_BEAM, MOVE_BLIZZARD, MOVE_TRI_ATTACK};

static u32 NaturalThawRoll(bool32 thaw)
{
    return B_POLISHED_MOVE_BALANCE ? (thaw ? 0 : 255) : thaw;
}

SINGLE_BATTLE_TEST("Polished freeze freshly applied status respects the next natural thaw roll")
{
    struct BattlePokemon *holder = player, *freezer = opponent;
    enum Move move = MOVE_ICE_PUNCH;
    bool32 first = FALSE, thaw = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sFreezeMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 order = 0; order < 2; order++)
                for (u32 roll = 0; roll < 2; roll++)
                    PARAMETRIZE { move = sFreezeMoves[m]; holder = side ? opponent : player; freezer = side ? player : opponent; first = order; thaw = roll; }
    bool32 canMove = !B_POLISHED_MOVE_BALANCE || thaw;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? first ? 150 : 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? first ? 150 : 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); }
    } WHEN {
        TURN { MOVE(freezer, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_RANDOM_FROM_LIST, 2)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
        if (first) TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(thaw))); }
    } SCENE {
        STATUS_ICON(holder, freeze: TRUE);
        if (canMove) {
            STATUS_ICON(holder, none: TRUE);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
        } else NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->status1, canMove ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], 10 - first - canMove);
        EXPECT_EQ(freezer->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished freeze has no forced thaw after repeated failed rolls")
{
    struct BattlePokemon *holder = player;
    enum Move move = MOVE_ICE_BEAM;
    PARAMETRIZE { holder = player; }
    PARAMETRIZE { holder = opponent; }
    GIVEN {
        WITH_CONFIG(B_FREEZE_TURNS, GEN_CHAMPIONS);
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); if (holder == player) Status1(STATUS1_FREEZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); if (holder == opponent) Status1(STATUS1_FREEZE); }
    } WHEN {
        for (u32 turn = 0; turn < 5; turn++)
            TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(FALSE))); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 10 : 7);
    }
}

SINGLE_BATTLE_TEST("Polished freeze uses the exact source byte boundary independently of generation presets")
{
    static const u32 rolls[] = {0, 50, 51, 52, 53, 255};
    struct BattlePokemon *holder = player;
    enum Move move = MOVE_ICE_BEAM;
    u32 roll = 0, config = GEN_9;
    for (u32 side = 0; side < 2; side++)
        for (u32 generation = 0; generation < 2; generation++)
            for (u32 n = 0; n < ARRAY_COUNT(rolls); n++)
                PARAMETRIZE { holder = side ? opponent : player; config = generation ? GEN_CHAMPIONS : GEN_9; roll = rolls[n]; }
    bool32 thaw = roll < 52;
    GIVEN {
        WITH_CONFIG(B_FREEZE_TURNS, config);
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); if (holder == player) Status1(STATUS1_FREEZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); if (holder == opponent) Status1(STATUS1_FREEZE); }
    } WHEN {
        TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, B_POLISHED_MOVE_BALANCE ? roll : thaw)); }
    } THEN {
        EXPECT_EQ(holder->status1, thaw ? STATUS1_NONE : STATUS1_FREEZE);
        EXPECT_EQ(holder->pp[0], thaw ? 9 : 10);
    }
}

SINGLE_BATTLE_TEST("Polished freeze preserves the exact natural thaw probability")
{
    struct BattlePokemon *holder = player;
    enum Move move = MOVE_ICE_BEAM;
    PARAMETRIZE { holder = player; }
    PARAMETRIZE { holder = opponent; }
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? 52 : 25, B_POLISHED_MOVE_BALANCE ? 256 : 100, RNG_FROZEN);
    GIVEN {
        WITH_CONFIG(B_FREEZE_TURNS, GEN_CHAMPIONS);
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); if (holder == player) Status1(STATUS1_FREEZE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); if (holder == opponent) Status1(STATUS1_FREEZE); }
    } WHEN {
        TURN { MOVE(holder, MOVE_TACKLE); }
    } SCENE {
        STATUS_ICON(holder, none: TRUE);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    }
}

SINGLE_BATTLE_TEST("Polished freeze persists through switching without forcing a thaw on return")
{
    struct BattlePokemon *holder = player, *freezer = opponent;
    enum Move move = MOVE_ICE_PUNCH;
    for (u32 m = 0; m < ARRAY_COUNT(sFreezeMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sFreezeMoves[m]; holder = side ? opponent : player; freezer = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 150 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 150 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(holder, MOVE_TACKLE); MOVE(freezer, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_RANDOM_FROM_LIST, 2)); }
        TURN { SWITCH(holder, 1); }
        TURN { SWITCH(holder, 0); }
        TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(FALSE))); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 9 : 8);
    }
}

SINGLE_BATTLE_TEST("Polished freeze still permits Lum and Aspear Berry cures before acting")
{
    struct BattlePokemon *holder = player, *freezer = opponent;
    enum Move move = MOVE_ICE_PUNCH;
    enum Item item = ITEM_ASPEAR_BERRY;
    for (u32 m = 0; m < ARRAY_COUNT(sFreezeMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 berry = 0; berry < 2; berry++)
                PARAMETRIZE { move = sFreezeMoves[m]; holder = side ? opponent : player; freezer = side ? player : opponent; item = berry ? ITEM_LUM_BERRY : ITEM_ASPEAR_BERRY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); Item(holder == player ? item : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); Item(holder == opponent ? item : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(freezer, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_RANDOM_FROM_LIST, 2)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(FALSE))); }
    } SCENE {
        STATUS_ICON(holder, freeze: TRUE);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_BERRY, holder);
        STATUS_ICON(holder, none: TRUE);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, holder);
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(holder->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished freeze a second application after berry consumption still checks the roll")
{
    struct BattlePokemon *holder = player, *freezer = opponent;
    enum Move move = MOVE_ICE_PUNCH;
    for (u32 m = 0; m < ARRAY_COUNT(sFreezeMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sFreezeMoves[m]; holder = side ? opponent : player; freezer = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); Item(holder == player ? ITEM_LUM_BERRY : ITEM_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); Item(holder == opponent ? ITEM_LUM_BERRY : ITEM_NONE); }
    } WHEN {
        TURN { MOVE(freezer, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_RANDOM_FROM_LIST, 2)); MOVE(holder, MOVE_TACKLE); }
        TURN { MOVE(freezer, move, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_RANDOM_FROM_LIST, 2)); MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(FALSE))); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->item, ITEM_NONE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 9 : 8);
    }
}

SINGLE_BATTLE_TEST("Polished freeze preserves source self-thaw moves and optional Flame Wheel")
{
    static const enum Move moves[] = {MOVE_SACRED_FIRE, MOVE_SCALD, MOVE_FLARE_BLITZ, MOVE_FLAME_WHEEL};
    struct BattlePokemon *holder = player, *freezer = opponent;
    enum Move move = MOVE_SACRED_FIRE;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = moves[m]; holder = side ? opponent : player; freezer = side ? player : opponent; }
    GIVEN {
        ASSUME(MoveThawsUser(move));
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 150 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}, {MOVE_ICE_BEAM, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 150 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}, {MOVE_ICE_BEAM, 10}); }
    } WHEN {
        TURN { MOVE(holder, MOVE_TACKLE); MOVE(freezer, MOVE_ICE_BEAM, secondaryEffect: TRUE); }
        TURN { MOVE(holder, move, hit: TRUE, secondaryEffect: FALSE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(FALSE))); }
    } SCENE {
        STATUS_ICON(holder, freeze: TRUE);
        STATUS_ICON(holder, none: TRUE);
        ANIMATION(ANIM_TYPE_MOVE, move, holder);
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->pp[1], 9);
    }
}

DOUBLE_BATTLE_TEST("Polished freeze leaves doubles natural thaw behavior native")
{
    struct BattlePokemon *holder = playerLeft, *freezer = opponentLeft;
    enum Move move = MOVE_ICE_PUNCH;
    for (u32 m = 0; m < ARRAY_COUNT(sFreezeMoves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sFreezeMoves[m]; holder = side ? opponentLeft : playerLeft; freezer = side ? playerLeft : opponentLeft; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == playerLeft ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); }
        PLAYER(SPECIES_WYNAUT) { Speed(75); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponentLeft ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); }
        OPPONENT(SPECIES_WYNAUT) { Speed(25); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(freezer, move, target: holder, hit: TRUE, secondaryEffect: TRUE, WITH_RNG(RNG_RANDOM_FROM_LIST, 2)); MOVE(holder, MOVE_TACKLE, target: freezer, WITH_RNG(RNG_FROZEN, FALSE)); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished freeze leaves either active Terastal native even for self-targeted actions")
{
    struct BattlePokemon *holder = player, *freezer = opponent;
    enum Move move = MOVE_ICE_PUNCH;
    bool32 holderTera = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sFreezeMoves); m++)
        for (u32 side = 0; side < 2; side++)
            for (u32 tera = 0; tera < 2; tera++)
                PARAMETRIZE { move = sFreezeMoves[m]; holder = side ? opponent : player; freezer = side ? player : opponent; holderTera = tera; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); MovesWithPP({MOVE_SPLASH, 10}, {move, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 50 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); TeraType(TYPE_NORMAL); MovesWithPP({MOVE_SPLASH, 10}, {move, 10}); }
    } WHEN {
        TURN { MOVE(freezer, move, hit: TRUE, secondaryEffect: TRUE, gimmick: holderTera ? GIMMICK_NONE : GIMMICK_TERA, WITH_RNG(RNG_RANDOM_FROM_LIST, 2)); MOVE(holder, MOVE_SPLASH, gimmick: holderTera ? GIMMICK_TERA : GIMMICK_NONE, WITH_RNG(RNG_FROZEN, FALSE)); }
    } THEN {
        EXPECT_EQ(holder->status1, STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished freeze applies the ordinary status rule after optional freezing attacks")
{
    static const enum Move moves[] = {MOVE_POWDER_SNOW, MOVE_ICE_FANG, MOVE_FREEZE_DRY};
    struct BattlePokemon *holder = player, *freezer = opponent;
    enum Move move = MOVE_POWDER_SNOW;
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = moves[m]; holder = side ? opponent : player; freezer = side ? player : opponent; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(holder == player ? 150 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(holder == opponent ? 150 : 100); MaxHP(1000); HP(1000); Attack(10); SpAttack(10); Ability(ABILITY_NONE); MovesWithPP({MOVE_TACKLE, 10}, {move, 10}); }
    } WHEN {
        // Act first to keep Ice Fang's separate flinch effect outside this test.
        TURN { MOVE(holder, MOVE_TACKLE); MOVE(freezer, move, hit: TRUE, secondaryEffect: TRUE); }
        TURN { MOVE(holder, MOVE_TACKLE, WITH_RNG(RNG_FROZEN, NaturalThawRoll(FALSE))); }
    } THEN {
        EXPECT_EQ(holder->status1, B_POLISHED_MOVE_BALANCE ? STATUS1_FREEZE : STATUS1_NONE);
        EXPECT_EQ(holder->pp[0], B_POLISHED_MOVE_BALANCE ? 9 : 8);
    }
}
