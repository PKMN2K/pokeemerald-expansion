#include "global.h"
#include "move.h"
#include "test/test.h"
#include "../src/data/polished_move_balance.h"

TEST("Polished move balance imports the twelve reviewed source rows")
{
#if B_POLISHED_MOVE_BALANCE
#define CHECK_MOVE(name) do { \
    EXPECT_EQ(GetMovePower(MOVE_##name), POLISHED_##name##_POWER); \
    EXPECT_EQ(GetMoveAccuracy(MOVE_##name), POLISHED_##name##_ACCURACY); \
    EXPECT_EQ(GetMoveType(MOVE_##name), POLISHED_##name##_TYPE); \
    EXPECT_EQ(GetMoveCategory(MOVE_##name), POLISHED_##name##_CATEGORY); \
} while (0)
    CHECK_MOVE(CUT);
    CHECK_MOVE(FLY);
    CHECK_MOVE(TAKE_DOWN);
    CHECK_MOVE(SING);
    CHECK_MOVE(SUPERSONIC);
    CHECK_MOVE(STRENGTH);
    CHECK_MOVE(FIRE_SPIN);
    CHECK_MOVE(DIG);
    CHECK_MOVE(SCALD);
    CHECK_MOVE(CRABHAMMER);
    CHECK_MOVE(OCTAZOOKA);
    CHECK_MOVE(WHIRLPOOL);
#undef CHECK_MOVE
#else
    EXPECT_EQ(GetMovePower(MOVE_CUT), 50);
    EXPECT_EQ(GetMoveAccuracy(MOVE_CUT), 95);
    EXPECT_EQ(GetMoveType(MOVE_CUT), TYPE_NORMAL);
    EXPECT_EQ(GetMoveType(MOVE_STRENGTH), TYPE_NORMAL);
    EXPECT_EQ(GetMovePower(MOVE_SCALD), 80);
#endif
    // Optional moves and deferred special cases retain their native definitions.
    EXPECT_EQ(GetMovePower(MOVE_LEAFAGE), 40);
    EXPECT_EQ(GetMoveType(MOVE_LEAFAGE), TYPE_GRASS);
    EXPECT_EQ(GetMovePower(MOVE_SONIC_BOOM), 1);
    EXPECT_EQ(GetMovePower(MOVE_DRAGON_RAGE), 1);
}
