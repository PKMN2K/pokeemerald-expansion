#include "global.h"
#include "battle.h"
#include "test/test.h"

TEST("Polished fixed damage separates runtime sentinel, imported payload and displayed power")
{
    const enum Move moves[] = {MOVE_SONIC_BOOM, MOVE_DRAGON_RAGE};
    const u32 damage[] = {20, 40};
    for (u32 i = 0; i < ARRAY_COUNT(moves); i++)
    {
        EXPECT_EQ(GetMoveEffect(moves[i]), EFFECT_FIXED_HP_DAMAGE);
        EXPECT_EQ(GetMovePower(moves[i]), 1);
        EXPECT_EQ(GetMoveFixedHPDamage(moves[i]), damage[i]);
        EXPECT_EQ(GetMoveDisplayPower(moves[i]), B_POLISHED_FIXED_DAMAGE ? damage[i] : 1);
        EXPECT_EQ(GetMoveCategory(moves[i]), DAMAGE_CATEGORY_SPECIAL);
    }
    EXPECT_EQ(GetMoveDisplayPower(MOVE_ABSORB), 20);
    EXPECT_EQ(GetMoveDisplayPower(MOVE_WATER_GUN), 40);
    EXPECT_EQ(GetMoveDisplayPower(MOVE_SEISMIC_TOSS), GetMovePower(MOVE_SEISMIC_TOSS));
    EXPECT_EQ(GetMoveDisplayPower(MOVE_SPLASH), 0);
    EXPECT_EQ(GetMoveDisplayPower(MOVE_NONE), GetMovePower(MOVE_NONE));
}
