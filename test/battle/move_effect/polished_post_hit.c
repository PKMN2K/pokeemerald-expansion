#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished command posthit Rage follows Anger Point and cannot boost after its critical cap")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *holder = enemy ? opponent : player;
    struct BattlePokemon *attacker = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_NONE : ABILITY_ANGER_POINT); Speed(enemy ? 50 : 100); HP(4000); MaxHP(4000); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!enemy ? ABILITY_NONE : ABILITY_ANGER_POINT); Speed(enemy ? 100 : 50); HP(4000); MaxHP(4000); }
    } WHEN {
        TURN { MOVE(holder, MOVE_RAGE); MOVE(attacker, MOVE_TACKLE, criticalHit: TRUE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, attacker);
        if (!B_POLISHED_MOVE_BALANCE) {
            if (enemy) MESSAGE("The opposing Wobbuffet's rage is building!");
            else MESSAGE("Wobbuffet's rage is building!");
        }
        ABILITY_POPUP(holder, ABILITY_ANGER_POINT);
        NONE_OF {
            MESSAGE("The opposing Wobbuffet's rage is building!");
            MESSAGE("Wobbuffet's rage is building!");
        }
    } THEN { EXPECT_EQ(holder->statStages[STAT_ATK], MAX_STAT_STAGE); }
}

SINGLE_BATTLE_TEST("Polished command posthit runs Justified then Air Balloon then Rage before later hit items")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *holder = enemy ? opponent : player;
    struct BattlePokemon *attacker = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(enemy ? ABILITY_NONE : ABILITY_JUSTIFIED); Speed(enemy ? 50 : 100); HP(4000); MaxHP(4000); if (!enemy) Item(ITEM_AIR_BALLOON); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(!enemy ? ABILITY_NONE : ABILITY_JUSTIFIED); Speed(enemy ? 100 : 50); HP(4000); MaxHP(4000); if (enemy) Item(ITEM_AIR_BALLOON); }
    } WHEN {
        TURN { MOVE(holder, MOVE_RAGE); MOVE(attacker, MOVE_BITE, secondaryEffect: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, attacker);
        if (!B_POLISHED_MOVE_BALANCE) {
            if (enemy) MESSAGE("The opposing Wobbuffet's rage is building!");
            else MESSAGE("Wobbuffet's rage is building!");
        }
        ABILITY_POPUP(holder, ABILITY_JUSTIFIED);
        if (enemy) MESSAGE("The opposing Wobbuffet's Air Balloon popped!");
        else MESSAGE("Wobbuffet's Air Balloon popped!");
        if (B_POLISHED_MOVE_BALANCE) {
            if (enemy) MESSAGE("The opposing Wobbuffet's rage is building!");
            else MESSAGE("Wobbuffet's rage is building!");
        }
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(holder->item, ITEM_NONE);
    }
}

SINGLE_BATTLE_TEST("Polished command posthit Rage uses the current Substitute after the breaking hit")
{
    bool32 enemy = FALSE, breaks = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 large = 0; large < 2; large++)
            PARAMETRIZE { enemy = side; breaks = large; }
    struct BattlePokemon *holder = enemy ? opponent : player;
    struct BattlePokemon *attacker = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 50 : 100); HP(enemy ? 4000 : 400); MaxHP(enemy ? 4000 : 400); Attack(breaks ? 1000 : 1); Defense(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(enemy ? 100 : 50); HP(!enemy ? 4000 : 400); MaxHP(!enemy ? 4000 : 400); Attack(breaks ? 1000 : 1); Defense(100); }
    } WHEN {
        TURN { MOVE(holder, MOVE_SUBSTITUTE); }
        TURN { MOVE(holder, MOVE_RAGE); MOVE(attacker, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, attacker);
        if (B_POLISHED_MOVE_BALANCE && breaks) {
            if (enemy) MESSAGE("The opposing Wobbuffet's rage is building!");
            else MESSAGE("Wobbuffet's rage is building!");
        } else NONE_OF {
            MESSAGE("The opposing Wobbuffet's rage is building!");
            MESSAGE("Wobbuffet's rage is building!");
        }
    } THEN {
        EXPECT_EQ(holder->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (B_POLISHED_MOVE_BALANCE && breaks));
        EXPECT_EQ(!!holder->volatiles.substitute, !breaks);
        EXPECT_EQ(holder->hp, 300);
    }
}
