#include "global.h"
#include "test/battle.h"

ASSUMPTIONS
{
    ASSUME(GetMovePower(MOVE_POLISHED_FURY_STRIKES) == 20);
    ASSUME(GetMoveAccuracy(MOVE_POLISHED_FURY_STRIKES) == 85);
    ASSUME(GetMovePP(MOVE_POLISHED_FURY_STRIKES) == 15);
    ASSUME(IsMultiHitMove(MOVE_POLISHED_FURY_STRIKES));
    ASSUME(GetMoveType(MOVE_POLISHED_HEALING_LIGHT) == TYPE_NORMAL);
    ASSUME(GetMovePP(MOVE_POLISHED_HEALING_LIGHT) == 5);
    ASSUME(GetMovePP(MOVE_POLISHED_FRESH_SNACK) == 10);
}

SINGLE_BATTLE_TEST("Polished Fury Strikes uses the source two-hit probability")
{
    PASSES_RANDOMLY(1, 3, RNG_HITS);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_POLISHED_FURY_STRIKES); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_POLISHED_FURY_STRIKES, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_POLISHED_FURY_STRIKES, player);
        MESSAGE("The Pokémon was hit 2 times!");
    }
}

SINGLE_BATTLE_TEST("Polished Fury Strikes uses the source five-hit probability")
{
    PASSES_RANDOMLY(1, 6, RNG_HITS);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_POLISHED_FURY_STRIKES); }
    } SCENE {
        for (u32 i = 0; i < 5; i++)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_POLISHED_FURY_STRIKES, player);
        MESSAGE("The Pokémon was hit 5 times!");
    }
}

SINGLE_BATTLE_TEST("Polished Fury Strikes preserves Skill Link and Loaded Dice")
{
    enum Ability ability;
    enum Item item;
    PARAMETRIZE { ability = ABILITY_SKILL_LINK; item = ITEM_NONE; }
    PARAMETRIZE { ability = ABILITY_SHADOW_TAG; item = ITEM_LOADED_DICE; }
    PASSES_RANDOMLY(item == ITEM_LOADED_DICE ? 1 : 2, 2, RNG_LOADED_DICE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); Item(item); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_POLISHED_FURY_STRIKES); }
    } SCENE {
        for (u32 i = 0; i < 5; i++)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_POLISHED_FURY_STRIKES, player);
        MESSAGE("The Pokémon was hit 5 times!");
    }
}

SINGLE_BATTLE_TEST("Polished Fresh Snack floors half HP independently of generation defaults")
{
    GIVEN {
        WITH_CONFIG(B_HALF_HP_HEALING_ROUNDING, GEN_9);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); MaxHP(201); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_POLISHED_FRESH_SNACK); }
    } SCENE {
        HP_BAR(player, damage: -100);
    }
}

SINGLE_BATTLE_TEST("Polished HealingLight uses source weather fractions and Umbrella rules")
{
    enum Move weatherMove;
    enum Item item;
    u32 healing;
    PARAMETRIZE { weatherMove = MOVE_CELEBRATE; item = ITEM_NONE; healing = 100; }
    PARAMETRIZE { weatherMove = MOVE_SUNNY_DAY; item = ITEM_NONE; healing = 132; }
    PARAMETRIZE { weatherMove = MOVE_RAIN_DANCE; item = ITEM_NONE; healing = 50; }
    PARAMETRIZE { weatherMove = MOVE_SANDSTORM; item = ITEM_NONE; healing = 50; }
    PARAMETRIZE { weatherMove = MOVE_HAIL; item = ITEM_NONE; healing = 50; }
    PARAMETRIZE { weatherMove = MOVE_SUNNY_DAY; item = ITEM_UTILITY_UMBRELLA; healing = 100; }
    PARAMETRIZE { weatherMove = MOVE_RAIN_DANCE; item = ITEM_UTILITY_UMBRELLA; healing = 100; }
    PARAMETRIZE { weatherMove = MOVE_SANDSTORM; item = ITEM_UTILITY_UMBRELLA; healing = 50; }
    PARAMETRIZE { weatherMove = MOVE_HAIL; item = ITEM_UTILITY_UMBRELLA; healing = 50; }
    GIVEN {
        WITH_CONFIG(B_TIME_OF_DAY_HEALING_MOVES, GEN_2);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); MaxHP(200); Item(item); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, weatherMove); MOVE(player, MOVE_POLISHED_HEALING_LIGHT); }
    } SCENE {
        HP_BAR(player, damage: -healing);
    }
}

SINGLE_BATTLE_TEST("Polished healing moves fail at full HP")
{
    enum Move move;
    PARAMETRIZE { move = MOVE_POLISHED_HEALING_LIGHT; }
    PARAMETRIZE { move = MOVE_POLISHED_FRESH_SNACK; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        NONE_OF { HP_BAR(player); }
        MESSAGE("Wobbuffet's HP is full!");
    }
}

AI_SINGLE_BATTLE_TEST("AI avoids Polished healing moves at full HP")
{
    enum Move move;
    PARAMETRIZE { move = MOVE_POLISHED_HEALING_LIGHT; }
    PARAMETRIZE { move = MOVE_POLISHED_FRESH_SNACK; }
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_POUND, move); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_POUND); }
    }
}

AI_SINGLE_BATTLE_TEST("AI uses Polished healing moves when recovery outheals incoming damage")
{
    enum Move move;
    PARAMETRIZE { move = MOVE_POLISHED_HEALING_LIGHT; }
    PARAMETRIZE { move = MOVE_POLISHED_FRESH_SNACK; }
    PASSES_RANDOMLY(100, 100, RNG_AI_SHOULD_RECOVER);
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_OMNISCIENT);
        PLAYER(SPECIES_LINOONE) { Speed(2); Moves(MOVE_HEADBUTT); }
        OPPONENT(SPECIES_GASTRODON) { Speed(5); Moves(MOVE_SCALD, move); HP(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_HEADBUTT); EXPECT_MOVE(opponent, move); }
    }
}
