#include "global.h"
#include "battle.h"
#include "battle_stat_change.h"
#include "test/test.h"
#include "test/polished_secondary_stat_drops.h"

TEST("Polished secondary stat drops bind all eighteen source chances and stat payloads")
{
    EXPECT_EQ(ARRAY_COUNT(sSourceStatDrops), 18);
    for (u32 i = 0; i < ARRAY_COUNT(sSourceStatDrops); i++)
    {
        enum Move move = sSourceStatDrops[i].move;
        EXPECT_EQ(GetMoveAdditionalEffectCount(move), 1);
        const struct AdditionalEffect *effect = GetMoveAdditionalEffectById(move, 0);
        EXPECT_EQ(effect->chance, !B_POLISHED_MOVE_BALANCE && move == MOVE_OCTAZOOKA ? 50 : sSourceStatDrops[i].chance);
        EXPECT_EQ(effect->moveEffect, MOVE_EFFECT_STAT_MINUS);
        EXPECT(!effect->self);
        EXPECT(!effect->preAttackEffect);
        EXPECT(!effect->onChargeTurnOnly);
        for (u32 stat = STAT_ATK; stat <= STAT_EVASION; stat++)
            EXPECT_EQ(GetStatStage(stat, effect), stat == sSourceStatDrops[i].stat ? 1 : 0);
    }
}
