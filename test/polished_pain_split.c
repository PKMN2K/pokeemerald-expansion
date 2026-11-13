#include "global.h"
#include "battle.h"
#include "test/test.h"

TEST("Polished Pain Split selects source data and bypass flags without changing native rules")
{
    EXPECT_EQ(GetMoveType(MOVE_PAIN_SPLIT), B_POLISHED_PAIN_SPLIT ? TYPE_GHOST : TYPE_NORMAL);
    EXPECT_EQ(GetMovePower(MOVE_PAIN_SPLIT), 0);
    EXPECT_EQ(GetMoveAccuracy(MOVE_PAIN_SPLIT), 0);
    EXPECT_EQ(GetMovePP(MOVE_PAIN_SPLIT), 20);
    EXPECT_EQ(GetMoveCategory(MOVE_PAIN_SPLIT), DAMAGE_CATEGORY_STATUS);
    EXPECT_EQ(MoveIgnoresSubstitute(MOVE_PAIN_SPLIT), B_POLISHED_PAIN_SPLIT);
    EXPECT_EQ(MoveIgnoresProtect(MOVE_PAIN_SPLIT), B_POLISHED_PAIN_SPLIT);
    EXPECT_EQ(MoveIgnoresSubstitute(MOVE_TACKLE), FALSE);
    EXPECT_EQ(MoveIgnoresProtect(MOVE_TACKLE), FALSE);
}
