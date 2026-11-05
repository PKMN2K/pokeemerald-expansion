#include "global.h"
#include "battle_setup.h"
#include "event_data.h"
#include "polished_sprout.h"
#include "test/test.h"

TEST("Sprout Tower preserves distinct source 3 percent and 2 percent encounter levels")
{
    const u8 sourceSlots[] = {0, 1, 0, 1, 2, 2, 3, 3, 4, 5, 6, 6};
    const u8 weights[] = {30, 30, 20, 10, 5, 3, 2};
    u32 counts[7] = {0};
    for (u32 roll = 0; roll < 100; roll++)
    {
        u32 slot = PolishedSproutLandSlot(roll);
        EXPECT_LT(slot, ARRAY_COUNT(sourceSlots));
        counts[sourceSlots[slot]]++;
    }
    for (u32 i = 0; i < ARRAY_COUNT(counts); i++)
        EXPECT_EQ(counts[i], weights[i]);
}

#if !IS_FRLG
TEST("Sprout Tower battles have independent persistent flags without colliding with partners or rewards")
{
    const u16 trainers[] = {TRAINER_POLISHED_SPROUT_CHOW, TRAINER_POLISHED_SPROUT_NICO,
        TRAINER_POLISHED_SPROUT_EDMOND, TRAINER_POLISHED_SPROUT_JIN,
        TRAINER_POLISHED_SPROUT_TROY, TRAINER_POLISHED_SPROUT_NEAL, TRAINER_POLISHED_SPROUT_LI};
    const u16 flags[] = {FLAG_POLISHED_SPROUT_CHOW, FLAG_POLISHED_SPROUT_NICO,
        FLAG_POLISHED_SPROUT_EDMOND, FLAG_POLISHED_SPROUT_JIN,
        FLAG_POLISHED_SPROUT_TROY, FLAG_POLISHED_SPROUT_NEAL, FLAG_POLISHED_SPROUT_LI};
    bool32 before[7];
    bool32 reward = FlagGet(FLAG_POLISHED_SPROUT_FLASH);
    bool32 rival = FlagGet(FLAG_POLISHED_SPROUT_RIVAL_LEFT);
    for (u32 i = 0; i < ARRAY_COUNT(trainers); i++)
    {
        EXPECT(IsRegularTrainerId(trainers[i]));
        EXPECT(!IsPartnerTrainerId(trainers[i]));
        EXPECT_EQ(GetTrainerFlagId(trainers[i]), flags[i]);
        before[i] = HasTrainerBeenFought(trainers[i]);
        ClearTrainerFlag(trainers[i]);
    }
    for (u32 i = 0; i < ARRAY_COUNT(trainers); i++)
    {
        SetTrainerFlag(trainers[i]);
        for (u32 j = 0; j < ARRAY_COUNT(trainers); j++)
            EXPECT_EQ(HasTrainerBeenFought(trainers[j]), j <= i);
    }
    EXPECT_EQ(GetTrainerFlagId(TRAINER_PARTNER(PARTNER_STEVEN)), 0);
    EXPECT_EQ(GetTrainerFlagId(TRAINERS_COUNT), 0);
    EXPECT_EQ(FlagGet(FLAG_POLISHED_SPROUT_FLASH), reward);
    EXPECT_EQ(FlagGet(FLAG_POLISHED_SPROUT_RIVAL_LEFT), rival);
    for (u32 i = 0; i < ARRAY_COUNT(trainers); i++)
        if (!before[i])
            ClearTrainerFlag(trainers[i]);
}
#endif
