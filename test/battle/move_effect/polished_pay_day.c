#include "global.h"
#include "test/battle.h"
#include "battle_scripts.h"
#include "battle_set_effect.h"

SINGLE_BATTLE_TEST("Polished Pay Day accumulates coins from either actor and rejects misses and Protect")
{
    bool32 enemy = FALSE;
    u32 rejection = 0;
    for (u32 side = 0; side < 2; side++)
        for (u32 block = 0; block < 3; block++)
            PARAMETRIZE { enemy = side; rejection = block; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(10); HP(1000); MaxHP(1000); Item(ITEM_BRIGHTPOWDER); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(10); HP(1000); MaxHP(1000); Item(ITEM_BRIGHTPOWDER); }
    } WHEN {
        TURN { MOVE(user, MOVE_PAY_DAY, hit: TRUE); }
        TURN { MOVE(user, MOVE_PAY_DAY, hit: rejection != 1); if (rejection == 2) MOVE(target, MOVE_PROTECT); }
    } THEN {
        EXPECT_EQ(gPaydayMoney, B_POLISHED_MOVE_BALANCE || !enemy ? 50 * (rejection ? 1 : 2) : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Pay Day checks the surviving Substitute and honors Infiltrator")
{
    bool32 enemy = FALSE, breaks = FALSE, infiltrator = FALSE;
    for (u32 side = 0; side < 2; side++)
        for (u32 broken = 0; broken < 2; broken++)
            for (u32 bypass = 0; bypass < 2; bypass++)
                PARAMETRIZE { enemy = side; breaks = broken; infiltrator = bypass; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(!enemy && infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE);
            Level(50); Attack(!enemy && breaks ? 999 : 10); Defense(50); HP(1000); MaxHP(1000);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Ability(enemy && infiltrator ? ABILITY_INFILTRATOR : ABILITY_NONE);
            Level(50); Attack(enemy && breaks ? 999 : 10); Defense(50); HP(1000); MaxHP(1000);
        }
    } WHEN {
        TURN { MOVE(target, MOVE_SUBSTITUTE); }
        TURN { MOVE(user, MOVE_PAY_DAY); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SUBSTITUTE, target);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PAY_DAY, user);
        if (infiltrator) HP_BAR(target);
        else SUB_HIT(target);
    } THEN {
        EXPECT_EQ((u32)target->volatiles.substitute, !breaks || infiltrator);
        EXPECT_EQ(gPaydayMoney, B_POLISHED_MOVE_BALANCE ? (breaks || infiltrator ? 250 : 0) : (!enemy ? 250 : 0));
    }
}

SINGLE_BATTLE_TEST("Polished Pay Day still accumulates coins when its damage faints the target")
{
    bool32 enemy = FALSE;
    PARAMETRIZE { enemy = FALSE; }
    PARAMETRIZE { enemy = TRUE; }
    struct BattlePokemon *user = enemy ? opponent : player;
    struct BattlePokemon *target = enemy ? player : opponent;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Speed(enemy ? 50 : 100); if (enemy) HP(1); }
        if (enemy) PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(50); Speed(enemy ? 100 : 50); if (!enemy) HP(1); }
        if (!enemy) OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Speed(50); }
    } WHEN {
        TURN { MOVE(user, MOVE_PAY_DAY); SEND_OUT(target, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PAY_DAY, user);
        HP_BAR(target);
        if (B_POLISHED_MOVE_BALANCE || !enemy) MESSAGE("Coins were scattered everywhere!");
    } THEN {
        EXPECT_EQ(gPaydayMoney, B_POLISHED_MOVE_BALANCE || !enemy ? 250 : 0);
    }
}

SINGLE_BATTLE_TEST("Polished Pay Day production effect wraps its source 24 bit accumulator and retains native caps")
{
    bool32 enemy = FALSE;
    u32 initial = 0;
    enum Move move = MOVE_PAY_DAY;
    const u32 values[] = {0, 0xFFFA, 0xFFFF, 0xFFFFFA, 0xFFFFFF};
    for (u32 side = 0; side < 2; side++)
        for (u32 value = 0; value < ARRAY_COUNT(values); value++)
            for (u32 native = 0; native < 2; native++)
                PARAMETRIZE { enemy = side; initial = values[value]; move = native ? MOVE_MAKE_IT_RAIN : MOVE_PAY_DAY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(10); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(10); }
    } WHEN { TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); } }
    THEN {
        bool32 source = B_POLISHED_MOVE_BALANCE && move == MOVE_PAY_DAY;
        // Invoke the production effect dispatcher to expose boundaries that
        // would otherwise require tens of thousands of real attack turns.
        gPaydayMoney = source ? initial : min(initial, 0xFFFF);
        u32 before = gPaydayMoney;
        gBattlerAttacker = enemy ? B_BATTLER_1 : B_BATTLER_0;
        gBattlerTarget = enemy ? B_BATTLER_0 : B_BATTLER_1;
        gCurrentMove = move;
        SetMoveEffectHelper(gBattlerAttacker, gBattlerTarget, MOVE_EFFECT_PAYDAY, BattleScript_MoveEnd, EFFECT_PRIMARY);
        EXPECT_EQ(gPaydayMoney, source ? (before + 50) & 0xFFFFFF : (enemy ? before : min(0xFFFF, before + 50)));
    }
}
