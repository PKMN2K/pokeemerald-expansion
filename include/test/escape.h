#ifndef GUARD_TEST_ESCAPE_H
#define GUARD_TEST_ESCAPE_H

#include "test/battle.h"

// A denied player RUN does not consume the turn; follow it with another action
// in the same TURN. WITH_RNG uses the normal per-action battle RNG machinery.
struct EscapeContext
{
    bool32 allowed;
    bool32 explicitAllowed;
    struct RiggedRNG rng;
    bool32 explicitRNG;
};

void TestRunBattle(u32 sourceLine, struct BattlePokemon *battler, struct EscapeContext ctx);
#define RUN(battler, ...) TestRunBattle(__LINE__, battler, (struct EscapeContext) { R_APPEND_TRUE(__VA_ARGS__) })

// Unlike ROAMING_BATTLE_TEST's two-sided replay, keep the real wild opponent
// controller and roaming AI. Use inside WILD_BATTLE_TEST's GIVEN block.
#define AUTONOMOUS_ROAMER() do { \
    gBattleTestRunnerState->data.recordedBattle.battleFlags |= BATTLE_TYPE_ROAMER; \
    gBattleTestRunnerState->data.recordedBattle.AI_scripts[B_BATTLER_1] = AI_FLAG_ROAMING; \
} while (0)

#endif
