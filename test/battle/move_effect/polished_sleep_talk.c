#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished Sleep Talk selection clears last move before subsequent Disable or Encore")
{
    bool32 enemy = FALSE;
    enum Move reaction = MOVE_DISABLE;
    for (u32 side = 0; side < 2; side++)
        for (u32 encore = 0; encore < 2; encore++)
            PARAMETRIZE { enemy = side; reaction = encore ? MOVE_ENCORE : MOVE_DISABLE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); MaxHP(1000); HP(1000); Speed(enemy ? 50 : 100);
            Status1(enemy ? STATUS1_NONE : STATUS1_SLEEP);
            if (!enemy) MovesWithPP({MOVE_SLEEP_TALK, 10}, {MOVE_TACKLE, 0});
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); MaxHP(1000); HP(1000); Speed(enemy ? 100 : 50);
            Status1(enemy ? STATUS1_SLEEP : STATUS1_NONE);
            if (enemy) MovesWithPP({MOVE_SLEEP_TALK, 10}, {MOVE_TACKLE, 0});
        }
    } WHEN {
        if (enemy)
            TURN { MOVE(opponent, MOVE_SLEEP_TALK); MOVE(player, reaction, hit: TRUE); }
        else
            TURN { MOVE(player, MOVE_SLEEP_TALK); MOVE(opponent, reaction, hit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, enemy ? opponent : player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, enemy ? opponent : player);
        HP_BAR(enemy ? player : opponent);
        if (!B_POLISHED_MOVE_BALANCE && reaction == MOVE_DISABLE)
            ANIMATION(ANIM_TYPE_MOVE, reaction, enemy ? player : opponent);
        else {
            NOT ANIMATION(ANIM_TYPE_MOVE, reaction, enemy ? player : opponent);
            MESSAGE("But it failed!");
        }
    } THEN {
        EXPECT_EQ((u32)(enemy ? opponent : player)->volatiles.disabledMove, !B_POLISHED_MOVE_BALANCE && reaction == MOVE_DISABLE ? MOVE_SLEEP_TALK : MOVE_NONE);
        EXPECT_EQ((u32)(enemy ? opponent : player)->volatiles.encoredMove, MOVE_NONE);
        EXPECT_EQ((enemy ? opponent : player)->pp[0], 9);
        EXPECT_EQ((enemy ? opponent : player)->pp[1], 0);
    }
}

SINGLE_BATTLE_TEST("Polished Sleep Talk selection clears last move on awake and no-candidate failure")
{
    bool32 asleep = FALSE;
    PARAMETRIZE { asleep = FALSE; }
    PARAMETRIZE { asleep = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Status1(asleep ? STATUS1_SLEEP : STATUS1_NONE); Moves(MOVE_SLEEP_TALK, MOVE_FLY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SLEEP_TALK); MOVE(opponent, MOVE_DISABLE, hit: TRUE); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
        MESSAGE("But it failed!");
        if (B_POLISHED_MOVE_BALANCE) {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, opponent);
            MESSAGE("But it failed!");
        } else
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, opponent);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.disabledMove, B_POLISHED_MOVE_BALANCE ? MOVE_NONE : MOVE_SLEEP_TALK);
        EXPECT_EQ(player->pp[0], 9);
    }
}

SINGLE_BATTLE_TEST("Polished Sleep Talk selection retains all six source excluded moves")
{
    enum Move excluded = MOVE_NONE;
    PARAMETRIZE { excluded = MOVE_METRONOME; }
    PARAMETRIZE { excluded = MOVE_SKETCH; }
    PARAMETRIZE { excluded = MOVE_SLEEP_TALK; }
    PARAMETRIZE { excluded = MOVE_SOLAR_BEAM; }
    PARAMETRIZE { excluded = MOVE_FLY; }
    PARAMETRIZE { excluded = MOVE_DIG; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Status1(STATUS1_SLEEP);
            if (excluded == MOVE_SLEEP_TALK) Moves(MOVE_SLEEP_TALK);
            else Moves(MOVE_SLEEP_TALK, excluded);
        }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, moveSlot: 0); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
        MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_SLEEP_TALK) - 1);
    }
}

SINGLE_BATTLE_TEST("Polished Sleep Talk selection excludes an already disabled move")
{
    bool32 alternative = FALSE;
    PARAMETRIZE { alternative = FALSE; }
    PARAMETRIZE { alternative = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Moves(MOVE_TACKLE, MOVE_SLEEP_TALK, alternative ? MOVE_DRAGON_DANCE : MOVE_FLY); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_DISABLE, hit: TRUE); }
        TURN { MOVE(opponent, MOVE_SPORE); }
        TURN { MOVE(player, MOVE_SLEEP_TALK); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPORE, opponent);
        if (alternative) {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, player);
        } else {
            NONE_OF {
                ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
                ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
            }
            MESSAGE("But it failed!");
        }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.disabledMove, MOVE_TACKLE);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + alternative);
    }
}

SINGLE_BATTLE_TEST("Polished Sleep Talk selection ignores Choice lock and empty called PP in every slot")
{
    u32 slot = 0;
    enum Item item = ITEM_NONE;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        for (u32 choice = 0; choice < 2; choice++)
            PARAMETRIZE { slot = i; item = choice ? ITEM_CHOICE_BAND : ITEM_NONE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Item(item); Status1(STATUS1_SLEEP);
            MovesWithPP({slot == 0 ? MOVE_TACKLE : MOVE_SLEEP_TALK, slot == 0 ? 0 : 10},
                        {slot == 1 ? MOVE_TACKLE : slot == 0 ? MOVE_SLEEP_TALK : MOVE_FLY, slot == 1 ? 0 : 10},
                        {slot == 2 ? MOVE_TACKLE : slot < 2 ? MOVE_FLY : MOVE_DIG, slot == 2 ? 0 : 10},
                        {slot == 3 ? MOVE_TACKLE : MOVE_DIG, slot == 3 ? 0 : 10});
        }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); MaxHP(1000); HP(1000); }
    } WHEN {
        TURN { MOVE(player, moveSlot: slot == 0 ? 1 : 0); }
        TURN { MOVE(player, moveSlot: slot == 0 ? 1 : 0); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(player->pp[slot], 0);
        EXPECT_EQ(player->pp[slot == 0 ? 1 : 0], 8);
    }
}

SINGLE_BATTLE_TEST("Polished Sleep Talk selection still fails awake and can select a self-targeting move")
{
    bool32 asleep = FALSE;
    PARAMETRIZE { asleep = FALSE; }
    PARAMETRIZE { asleep = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Status1(asleep ? STATUS1_SLEEP : STATUS1_NONE); Moves(MOVE_SLEEP_TALK, MOVE_DRAGON_DANCE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SLEEP_TALK); }
    } SCENE {
        if (asleep) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, player);
        } else {
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
            MESSAGE("But it failed!");
        }
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + asleep);
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + asleep);
        EXPECT_EQ(player->pp[0], 9);
        EXPECT_EQ(player->pp[1], GetMovePP(MOVE_DRAGON_DANCE));
    }
}

SINGLE_BATTLE_TEST("Polished Sleep Talk selection clears history even when its called attack misses or is protected")
{
    bool32 protect = FALSE;
    PARAMETRIZE { protect = FALSE; }
    PARAMETRIZE { protect = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Status1(STATUS1_SLEEP); Moves(MOVE_SLEEP_TALK, MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); }
    } WHEN {
        if (!protect)
            TURN { MOVE(opponent, MOVE_DOUBLE_TEAM); }
        if (protect)
            TURN { MOVE(opponent, MOVE_PROTECT); MOVE(player, MOVE_SLEEP_TALK); }
        else
            TURN { MOVE(player, MOVE_SLEEP_TALK, WITH_RNG(RNG_ACCURACY, FALSE)); }
        TURN { MOVE(opponent, MOVE_DISABLE, hit: TRUE); }
    } SCENE {
        if (!protect)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DOUBLE_TEAM, opponent);
        if (protect)
            ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SLEEP_TALK, player);
        if (B_POLISHED_MOVE_BALANCE) {
            NONE_OF {
                HP_BAR(opponent);
                ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, opponent);
            }
            MESSAGE("But it failed!");
        } else {
            NOT HP_BAR(opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, opponent);
        }
    } THEN {
        EXPECT_EQ((u32)player->volatiles.disabledMove, B_POLISHED_MOVE_BALANCE ? MOVE_NONE : MOVE_SLEEP_TALK);
        EXPECT_EQ(player->pp[0], 9);
        EXPECT_EQ(player->pp[1], GetMovePP(MOVE_TACKLE));
    }
}

SINGLE_BATTLE_TEST("Polished Sleep Talk selection restores ordinary move history on the next action")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Moves(MOVE_SLEEP_TALK, MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(player, MOVE_SLEEP_TALK); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_DISABLE, hit: TRUE); }
    } SCENE {
        MESSAGE("But it failed!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, opponent);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.disabledMove, MOVE_TACKLE);
    }
}
