#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"

SINGLE_BATTLE_TEST("Polished self move accuracy does not give Dragon Dance a hit roll")
{
    u32 modifier = 0;
    PARAMETRIZE { modifier = 0; }
    PARAMETRIZE { modifier = 1; }
    PARAMETRIZE { modifier = 2; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_BRIGHT_POWDER); Moves(MOVE_DRAGON_DANCE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_BRIGHT_POWDER); }
    } WHEN {
        for (u32 i = 0; i < 6 && modifier < 2; i++)
        {
            if (modifier == 0)
                TURN { MOVE(opponent, MOVE_SAND_ATTACK, WITH_RNG(RNG_ACCURACY, TRUE)); }
            else
                TURN { MOVE(opponent, MOVE_DOUBLE_TEAM); }
        }
        TURN { MOVE(player, MOVE_DRAGON_DANCE, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
    } THEN {
        if (modifier == 0)
            EXPECT_EQ(player->statStages[STAT_ACC], MIN_STAT_STAGE);
        if (modifier == 1)
            EXPECT_EQ(opponent->statStages[STAT_EVASION], MAX_STAT_STAGE);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(GetMonData(GetBattlerMon(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)), MON_DATA_PP1), 19);
    }
}

SINGLE_BATTLE_TEST("Polished self move accuracy retains Dragon Dance partial and full stat caps")
{
    enum Move setup = MOVE_SWORDS_DANCE;
    PARAMETRIZE { setup = MOVE_SWORDS_DANCE; }
    PARAMETRIZE { setup = MOVE_AGILITY; }
    PARAMETRIZE { setup = MOVE_DRAGON_DANCE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        for (u32 i = 0; i < (setup == MOVE_DRAGON_DANCE ? 6 : 3); i++)
            TURN { MOVE(player, setup); }
        TURN { MOVE(player, MOVE_DRAGON_DANCE); }
    } SCENE {
        for (u32 i = 0; i < (setup == MOVE_DRAGON_DANCE ? 6 : 3); i++)
            ANIMATION(ANIM_TYPE_MOVE, setup, player);
        if (setup == MOVE_DRAGON_DANCE)
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, player);
        else
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, player);
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], setup == MOVE_AGILITY ? DEFAULT_STAT_STAGE + 1 : MAX_STAT_STAGE);
        EXPECT_EQ(player->statStages[STAT_SPEED], setup == MOVE_SWORDS_DANCE ? DEFAULT_STAT_STAGE + 1 : MAX_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished self move accuracy keeps Dragon Dance independent of opponent protection and hiding")
{
    enum Move move = MOVE_PROTECT;
    PARAMETRIZE { move = MOVE_PROTECT; }
    PARAMETRIZE { move = MOVE_FLY; }
    PARAMETRIZE { move = MOVE_DIG; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_GASTLY) { Speed(100); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponent, move); MOVE(player, MOVE_DRAGON_DANCE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Polished self move accuracy lets Sleep Talk call Dragon Dance without a parent hit roll")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_SLEEP); Ability(ABILITY_NONE); MovesWithPP({MOVE_SLEEP_TALK, 10}, {MOVE_DRAGON_DANCE, 0}); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_BRIGHT_POWDER); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SAND_ATTACK); }
        TURN { MOVE(player, MOVE_SLEEP_TALK, WITH_RNG(RNG_ACCURACY, FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
        struct Pokemon *mon = GetBattlerMon(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT));
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP1), 9);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP2), 0);
    }
}

SINGLE_BATTLE_TEST("Polished self move accuracy makes Sleep Talk use its called move's hit rules")
{
    enum Move called = MOVE_TACKLE;
    bool32 hit = FALSE;
    PARAMETRIZE { called = MOVE_TACKLE; hit = FALSE; }
    PARAMETRIZE { called = MOVE_TACKLE; hit = TRUE; }
    PARAMETRIZE { called = MOVE_SWIFT; hit = FALSE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_SLEEP); Ability(ABILITY_NONE); MovesWithPP({MOVE_SLEEP_TALK, 10}, {called, 0}); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Ability(ABILITY_NONE); Item(ITEM_BRIGHT_POWDER); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_DOUBLE_TEAM); }
        TURN { MOVE(player, MOVE_SLEEP_TALK, WITH_RNG(RNG_ACCURACY, hit)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
        if (hit || called == MOVE_SWIFT)
        {
            ANIMATION(ANIM_TYPE_MOVE, called, player);
            HP_BAR(opponent);
        }
        else
            NONE_OF { ANIMATION(ANIM_TYPE_MOVE, called, player); HP_BAR(opponent); }
    } THEN {
        struct Pokemon *mon = GetBattlerMon(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT));
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP1), 9);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP2), 0);
    }
}

SINGLE_BATTLE_TEST("Polished self move accuracy preserves protection and type immunity for Sleep Talk's called move")
{
    bool32 ghost = FALSE;
    PARAMETRIZE { ghost = FALSE; }
    PARAMETRIZE { ghost = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_SLEEP); Moves(MOVE_SLEEP_TALK, MOVE_POUND); }
        OPPONENT(ghost ? SPECIES_GASTLY : SPECIES_WOBBUFFET) { HP(500); MaxHP(500); }
    } WHEN {
        TURN { MOVE(opponent, ghost ? MOVE_SPLASH : MOVE_PROTECT); MOVE(player, MOVE_SLEEP_TALK); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_POUND, player); HP_BAR(opponent); }
    }
}

SINGLE_BATTLE_TEST("Polished self move accuracy retains Sleep Talk's awake and no-eligible-move failures")
{
    bool32 asleep = FALSE;
    PARAMETRIZE { asleep = FALSE; }
    PARAMETRIZE { asleep = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Status1(asleep ? STATUS1_SLEEP : STATUS1_NONE); Moves(MOVE_SLEEP_TALK, asleep ? MOVE_FLY : MOVE_TACKLE, MOVE_DIG); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SLEEP_TALK); }
    } SCENE {
        MESSAGE("But it failed!");
        NONE_OF { ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player); HP_BAR(opponent); }
    }
}

AI_SINGLE_BATTLE_TEST("Polished self move accuracy AI does not treat listed parent accuracy as a hit roll")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY);
        PLAYER(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Ability(ABILITY_NONE); Item(ITEM_BRIGHT_POWDER); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_DRAGON_DANCE, MOVE_SLEEP_TALK, MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); }
    } THEN {
        enum BattlerId attacker = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        enum BattlerId defender = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        opponent->statStages[STAT_ACC] = MIN_STAT_STAGE;
        player->statStages[STAT_EVASION] = MAX_STAT_STAGE;
        CalcBattlerAiMovesData(gAiLogicData, attacker, defender, AI_GetWeather(), gFieldStatuses);
        // The engine's 101 sentinel means no ordinary accuracy calculation.
        EXPECT_EQ(gAiLogicData->moveAccuracy[attacker][defender][0], 101);
        EXPECT_EQ(gAiLogicData->moveAccuracy[attacker][defender][1], 101);
        EXPECT_LT(gAiLogicData->moveAccuracy[attacker][defender][2], 100);
        EXPECT_GT(gAiLogicData->moveAccuracy[attacker][defender][2], 0);
    }
}
