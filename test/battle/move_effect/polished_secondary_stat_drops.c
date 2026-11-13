#include "global.h"
#include "test/battle.h"
#include "test/polished_secondary_stat_drops.h"

SINGLE_BATTLE_TEST("Polished secondary stat drops lower exactly the selected target stat by one on both sides")
{
    enum Move move = MOVE_NONE;
    enum Stat stat = STAT_ATK;
    bool32 enemy = FALSE;
    for (u32 m = 0; m < ARRAY_COUNT(sSourceStatDrops); m++)
        for (u32 side = 0; side < 2; side++)
            PARAMETRIZE { move = sSourceStatDrops[m].move; stat = sSourceStatDrops[m].stat; enemy = side; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        if (enemy)
            TURN { MOVE(opponent, move, hit: TRUE, WITH_RNG(RNG_SECONDARY_EFFECT, TRUE)); }
        else
            TURN { MOVE(player, move, hit: TRUE, WITH_RNG(RNG_SECONDARY_EFFECT, TRUE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, enemy ? opponent : player);
        HP_BAR(enemy ? player : opponent);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, enemy ? player : opponent);
    } THEN {
        for (u32 s = STAT_ATK; s <= STAT_EVASION; s++)
        {
            EXPECT_EQ((enemy ? player : opponent)->statStages[s], DEFAULT_STAT_STAGE - (s == stat));
            EXPECT_EQ((enemy ? opponent : player)->statStages[s], DEFAULT_STAT_STAGE);
        }
    }
}

SINGLE_BATTLE_TEST("Polished secondary stat drops use source probabilities and Serene Grace doubling")
{
    enum Move move = MOVE_NONE;
    enum Ability ability = ABILITY_NONE;
    enum Stat stat = STAT_ATK;
    u32 chance = 0;
    for (u32 m = 0; m < ARRAY_COUNT(sSourceStatDrops); m++)
        if (sSourceStatDrops[m].chance < 100)
            for (u32 grace = 0; grace < 2; grace++)
                PARAMETRIZE {
                    move = sSourceStatDrops[m].move;
                    stat = sSourceStatDrops[m].stat;
                    ability = grace ? ABILITY_SERENE_GRACE : ABILITY_NONE;
                    chance = (!B_POLISHED_MOVE_BALANCE && move == MOVE_OCTAZOOKA ? 50 : sSourceStatDrops[m].chance) * (1 + grace);
                }
    PASSES_RANDOMLY(chance, 100, RNG_SECONDARY_EFFECT);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ability); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, move, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponent);
    } THEN {
        EXPECT_EQ(opponent->statStages[stat], DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Polished secondary stat drops respect Shield Dust Covert Cloak Sheer Force and Clear Body")
{
    enum Move move = MOVE_OCTAZOOKA;
    enum Stat stat = STAT_ACC;
    enum Ability attacker = ABILITY_NONE, defender = ABILITY_NONE;
    enum Item item = ITEM_NONE;
    for (u32 m = 0; m < 2; m++)
        for (u32 gate = 0; gate < 4; gate++)
            PARAMETRIZE {
                move = m ? MOVE_ICY_WIND : MOVE_OCTAZOOKA;
                stat = m ? STAT_SPEED : STAT_ACC;
                attacker = gate == 2 ? ABILITY_SHEER_FORCE : ABILITY_NONE;
                defender = gate == 0 ? ABILITY_SHIELD_DUST : gate == 3 ? ABILITY_CLEAR_BODY : ABILITY_NONE;
                item = gate == 1 ? ITEM_COVERT_CLOAK : ITEM_NONE;
            }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(attacker); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(defender); Item(item); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, move, hit: TRUE, WITH_RNG(RNG_SECONDARY_EFFECT, TRUE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponent);
    } THEN {
        EXPECT_EQ(opponent->statStages[stat], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished secondary stat drops let Mold Breaker bypass defensive abilities but not Covert Cloak")
{
    enum Ability ability = ABILITY_NONE;
    enum Item item = ITEM_NONE;
    PARAMETRIZE { ability = ABILITY_SHIELD_DUST; }
    PARAMETRIZE { ability = ABILITY_CLEAR_BODY; }
    PARAMETRIZE { item = ITEM_COVERT_CLOAK; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_MOLD_BREAKER); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ability); Item(item); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_OCTAZOOKA, hit: TRUE, WITH_RNG(RNG_SECONDARY_EFFECT, TRUE)); }
    } SCENE {
        HP_BAR(opponent);
        if (item == ITEM_NONE)
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponent);
        else
            NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponent);
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ACC], DEFAULT_STAT_STAGE - (item == ITEM_NONE));
    }
}

SINGLE_BATTLE_TEST("Polished secondary stat drops do not pass through substitutes even when the substitute breaks")
{
    u32 hp = 1000;
    PARAMETRIZE { hp = 1000; }
    PARAMETRIZE { hp = 8; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(hp); HP(hp); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SUBSTITUTE); }
        TURN { MOVE(player, MOVE_OCTAZOOKA, hit: TRUE, WITH_RNG(RNG_SECONDARY_EFFECT, TRUE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_OCTAZOOKA, player);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponent);
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponent->hp, hp - hp / 4);
    }
}

SINGLE_BATTLE_TEST("Polished secondary stat drops fail after a miss protection immunity or target fainting")
{
    u32 gate = 0;
    for (u32 n = 0; n < 4; n++)
        PARAMETRIZE { gate = n; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(gate == 2 ? ABILITY_WATER_ABSORB : ABILITY_NONE); MaxHP(1000); HP(gate == 3 ? 1 : 1000); }
    } WHEN {
        if (gate == 1)
            TURN { MOVE(opponent, MOVE_PROTECT); MOVE(player, MOVE_OCTAZOOKA, WITH_RNG(RNG_SECONDARY_EFFECT, TRUE)); }
        else
            TURN { MOVE(player, MOVE_OCTAZOOKA, hit: gate != 0, WITH_RNG(RNG_SECONDARY_EFFECT, TRUE)); }
    } SCENE {
        if (gate == 3)
            HP_BAR(opponent, hp: 0);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponent);
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ACC], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Polished secondary stat drops stop at the stat floor and preserve parent PP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_OCTAZOOKA, MOVE_SAND_ATTACK); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        for (u32 i = 0; i < 6; i++)
            TURN { MOVE(player, MOVE_SAND_ATTACK); }
        TURN { MOVE(player, MOVE_OCTAZOOKA, hit: TRUE, WITH_RNG(RNG_SECONDARY_EFFECT, TRUE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_OCTAZOOKA, player);
        HP_BAR(opponent);
        NOT ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponent);
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ACC], MIN_STAT_STAGE);
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_OCTAZOOKA) - 1);
    }
}
