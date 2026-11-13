#include "global.h"
#include "battle.h"
#include "test/test.h"

TEST("Polished retaliation accuracy imports the bypass without changing damage identity")
{
    const enum Move moves[] = {MOVE_COUNTER, MOVE_MIRROR_COAT};
    const enum Type types[] = {TYPE_FIGHTING, TYPE_PSYCHIC};
    const enum DamageCategory categories[] = {DAMAGE_CATEGORY_PHYSICAL, DAMAGE_CATEGORY_SPECIAL};
    for (u32 i = 0; i < ARRAY_COUNT(moves); i++)
    {
        EXPECT_EQ(GetMoveAccuracy(moves[i]), B_POLISHED_RETALIATION_ACCURACY ? 0 : 100);
        EXPECT_EQ(GetMoveEffect(moves[i]), EFFECT_REFLECT_DAMAGE);
        EXPECT_EQ(GetMovePower(moves[i]), 1);
        EXPECT_EQ(GetMoveType(moves[i]), types[i]);
        EXPECT_EQ(GetMoveCategory(moves[i]), categories[i]);
        EXPECT_EQ(GetMovePP(moves[i]), 20);
        EXPECT_EQ(GetMovePriority(moves[i]), -5);
        EXPECT_EQ(GetMoveReflectDamage_DamagePercent(moves[i]), 200);
        EXPECT_EQ(GetMoveReflectDamage_DamageCategories(moves[i]), 1u << categories[i]);
    }
    EXPECT_EQ(GetMoveAccuracy(MOVE_METAL_BURST), 100);
    EXPECT_EQ(GetMoveAccuracy(MOVE_TACKLE), 100);
}
