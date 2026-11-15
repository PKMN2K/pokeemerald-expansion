#include "global.h"
#include "move.h"
#include "test/test.h"
#include "test/polished_magic_bounce.h"

TEST("Polished Magic Bounce candidate flags match all 255 source effect scripts")
{
    u32 candidates = 0;
    EXPECT_EQ(ARRAY_COUNT(sSourceMagicBounceMoves), 255);
    for (u32 i = 0; i < ARRAY_COUNT(sSourceMagicBounceMoves); i++)
    {
        EXPECT_EQ(MoveCanBeBouncedBack(sSourceMagicBounceMoves[i].move), sSourceMagicBounceMoves[i].candidate);
        candidates += sSourceMagicBounceMoves[i].candidate;
    }
    EXPECT_EQ(candidates, 31);
}
