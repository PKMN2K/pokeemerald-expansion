#include "global.h"
#include "battle.h"
#include "test/test.h"

TEST("Polished self move accuracy imports listed data without changing effect identity")
{
    const enum Move moves[] = {MOVE_DRAGON_DANCE, MOVE_SLEEP_TALK};
    const enum Type types[] = {TYPE_DRAGON, TYPE_NORMAL};
    const u32 pp[] = {20, 10};
    for (u32 i = 0; i < ARRAY_COUNT(moves); i++)
    {
        EXPECT_EQ(GetMoveAccuracy(moves[i]), B_POLISHED_MOVE_BALANCE ? 100 : 0);
        EXPECT_EQ(GetMovePower(moves[i]), 0);
        EXPECT_EQ(GetMoveType(moves[i]), types[i]);
        EXPECT_EQ(GetMoveCategory(moves[i]), DAMAGE_CATEGORY_STATUS);
        EXPECT_EQ(GetMovePP(moves[i]), pp[i]);
        EXPECT_EQ(GetMovePriority(moves[i]), 0);
    }
    EXPECT_EQ(GetMoveEffect(MOVE_DRAGON_DANCE), EFFECT_STAT_CHANGE);
    EXPECT_EQ(GetMoveTarget(MOVE_DRAGON_DANCE), TARGET_USER);
    EXPECT_EQ(GetMoveEffect(MOVE_SLEEP_TALK), EFFECT_SLEEP_TALK);
    EXPECT_EQ(GetMoveTarget(MOVE_SLEEP_TALK), TARGET_DEPENDS);
    EXPECT_EQ(GetMoveAccuracy(MOVE_SWIFT), 0);
    EXPECT_EQ(GetMoveAccuracy(MOVE_WATER_GUN), 100);
}
