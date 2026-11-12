#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished move balance uses selected base power in real damaging turns")
{
    enum Move move = MOVE_NONE;
    u32 power = 0;
    const enum Move moves[] = {MOVE_CUT, MOVE_FLY, MOVE_TAKE_DOWN, MOVE_STRENGTH, MOVE_FIRE_SPIN,
        MOVE_DIG, MOVE_SCALD, MOVE_CRABHAMMER, MOVE_OCTAZOOKA, MOVE_WHIRLPOOL};
    const u8 powers[] = {60, 90, 90, 80, 40, 90, 70, 100, 75, 40};
    for (u32 m = 0; m < ARRAY_COUNT(moves); m++)
    {
        PARAMETRIZE { move = moves[m]; power = B_POLISHED_MOVE_BALANCE ? powers[m] : gMovesInfo[moves[m]].power; }
    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, move); }
        if (move == MOVE_FLY || move == MOVE_DIG)
            TURN { SKIP_TURN(player); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(gBattleMovePower, power);
    }
}

SINGLE_BATTLE_TEST("Polished move balance uses the selected accuracy thresholds in battle")
{
    enum Move move;
    u32 accuracy;
    PARAMETRIZE { move = MOVE_CUT; accuracy = 100; }
    PARAMETRIZE { move = MOVE_FLY; accuracy = 100; }
    PARAMETRIZE { move = MOVE_TAKE_DOWN; accuracy = 100; }
    PARAMETRIZE { move = MOVE_SING; accuracy = 75; }
    PARAMETRIZE { move = MOVE_SUPERSONIC; accuracy = 65; }
    PARAMETRIZE { move = MOVE_FIRE_SPIN; accuracy = 90; }
    PARAMETRIZE { move = MOVE_CRABHAMMER; accuracy = 95; }
    PARAMETRIZE { move = MOVE_OCTAZOOKA; accuracy = 95; }
    PARAMETRIZE { move = MOVE_WHIRLPOOL; accuracy = 90; }
    // This checks the imported threshold in the existing engine. Source byte
    // RNG/rounding and ability/item modifiers remain separate parity work.
    PASSES_RANDOMLY(B_POLISHED_MOVE_BALANCE ? accuracy : GetMoveAccuracy(move), 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, move); }
        if (move == MOVE_FLY)
            TURN { SKIP_TURN(player); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        // Fly also animates while charging, even when the second turn misses.
        if (move == MOVE_FLY)
            HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Polished move balance gives Cut Steel and Strength Fighting effectiveness")
{
    enum Move move;
    enum Species target;
    PARAMETRIZE { move = MOVE_CUT; target = SPECIES_CLEFAIRY; }
    PARAMETRIZE { move = MOVE_STRENGTH; target = SPECIES_RATTATA; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(target) { HP(1000); MaxHP(1000); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent);
        if (B_POLISHED_MOVE_BALANCE)
            MESSAGE("It's super effective!");
        else
            NOT MESSAGE("It's super effective!");
    }
}

SINGLE_BATTLE_TEST("Polished move balance allows Steel Cut to hit Ghost targets")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_GASTLY);
    } WHEN {
        TURN { MOVE(player, MOVE_CUT); }
    } SCENE {
        if (B_POLISHED_MOVE_BALANCE)
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CUT, player);
            HP_BAR(opponent);
        }
        else
        {
            MESSAGE("It doesn't affect the opposing Gastly…");
            NONE_OF { HP_BAR(opponent); }
        }
    }
}

AI_SINGLE_BATTLE_TEST("Polished move balance AI evaluates source Cut power and type")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | AI_FLAG_CHECK_VIABILITY);
        PLAYER(SPECIES_CLEFAIRY) { HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_MAWILE) { Moves(MOVE_CUT, MOVE_WING_ATTACK); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, B_POLISHED_MOVE_BALANCE ? MOVE_CUT : MOVE_WING_ATTACK); }
    }
}
