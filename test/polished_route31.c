#include "global.h"
#include "battle_setup.h"
#include "data.h"
#include "event_data.h"
#include "test/test.h"

#if !IS_FRLG
TEST("Route 31 trainer victories preserve partner IDs and neighboring story flags")
{
    const u16 trainers[] = {TRAINER_POLISHED_WADE, TRAINER_POLISHED_FINCH};
    const u16 flags[] = {FLAG_POLISHED_ROUTE31_WADE, FLAG_POLISHED_ROUTE31_FINCH};
    EXPECT_EQ(TRAINER_PARTNER(PARTNER_STEVEN), 865);
    EXPECT_EQ(GetTrainerFlagId(TRAINER_POLISHED_JOEY), 0x85F);
    EXPECT_EQ(FLAG_SYS_POKEMON_GET, 0x860);
    EXPECT_EQ(GetTrainerFlagId(TRAINER_PARTNER(PARTNER_STEVEN)), 0);
    EXPECT_EQ(GetTrainerFlagId(TRAINERS_COUNT), 0);
    bool32 original[2];
    for (u32 i = 0; i < ARRAY_COUNT(trainers); i++)
    {
        EXPECT(IsRegularTrainerId(trainers[i]));
        EXPECT(!IsPartnerTrainerId(trainers[i]));
        EXPECT_EQ(GetTrainerFlagId(trainers[i]), flags[i]);
        original[i] = HasTrainerBeenFought(trainers[i]);
        ClearTrainerFlag(trainers[i]);
    }
    bool32 starter = FlagGet(FLAG_SYS_POKEMON_GET);
    SetTrainerFlag(trainers[0]);
    EXPECT(HasTrainerBeenFought(trainers[0]));
    EXPECT(!HasTrainerBeenFought(trainers[1]));
    SetTrainerFlag(trainers[1]);
    ClearTrainerFlag(trainers[0]);
    EXPECT(!HasTrainerBeenFought(trainers[0]));
    EXPECT(HasTrainerBeenFought(trainers[1]));
    EXPECT_EQ(FlagGet(FLAG_SYS_POKEMON_GET), starter);
    for (u32 i = 0; i < ARRAY_COUNT(trainers); i++)
        if (original[i])
            SetTrainerFlag(trainers[i]);
        else
            ClearTrainerFlag(trainers[i]);
}

TEST("Route 31 Finch scales both party levels with badges while Wade stays fixed")
{
    const u8 baseLevels[] = {4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 43, 46, 49, 52, 55, 58, 61};
    u16 original = VarGet(VAR_POLISHED_EARNED_BADGES);
    bool32 surf = FlagGet(FLAG_BADGE05_GET);
    FlagSet(FLAG_BADGE05_GET); // Previous demo saves already grant Surf in the garden.
    for (u32 count = 0; count < ARRAY_COUNT(baseLevels); count++)
    {
        VarSet(VAR_POLISHED_EARNED_BADGES, (1u << count) - 1);
        EXPECT_EQ(GetPolishedTrainerLevel(TRAINER_POLISHED_FINCH, 8), baseLevels[count] + 4);
        EXPECT_EQ(GetPolishedTrainerLevel(TRAINER_POLISHED_FINCH, 9), baseLevels[count] + 5);
        EXPECT_EQ(GetPolishedTrainerLevel(TRAINER_POLISHED_WADE, 3), 3);
    }
    VarSet(VAR_POLISHED_EARNED_BADGES, (1u << 1) | (1u << 14));
    EXPECT_EQ(GetPolishedTrainerLevel(TRAINER_POLISHED_FINCH, 8), 16);
    VarSet(VAR_POLISHED_EARNED_BADGES, original);
    if (!surf)
        FlagClear(FLAG_BADGE05_GET);
}
#endif
