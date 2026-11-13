#include "global.h"
#include "battle.h"
#include "test/test.h"
#include "test/polished_sleep_talk.h"

TEST("Polished Sleep Talk selection matches all source known-move effect exclusions")
{
    EXPECT_EQ(ARRAY_COUNT(sSourceSleepTalkMoves), 254);
    for (u32 i = 0; i < ARRAY_COUNT(sSourceSleepTalkMoves); i++)
    {
        enum Move move = sSourceSleepTalkMoves[i].move;
        bool32 eligible = !IsMoveSleepTalkBanned(move) && !gBattleMoveEffects[GetMoveEffect(move)].twoTurnEffect;
        EXPECT_EQ(eligible, sSourceSleepTalkMoves[i].eligible);
    }
    // Struggle is not a legal source known move; retain the native guard.
    EXPECT(IsMoveSleepTalkBanned(MOVE_STRUGGLE));
}
