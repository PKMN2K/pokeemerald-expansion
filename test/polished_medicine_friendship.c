#include "global.h"
#include "battle.h"
#include "overworld.h"
#include "polished_items.h"
#include "test/test.h"

static const struct { enum Item item; s8 low, high; u16 healedHP; } sMedicine[] = {
    {ITEM_ENERGY_POWDER, -5, -10, 61},
    {ITEM_ENERGY_ROOT, -10, -15, 121},
    {ITEM_HEAL_POWDER, -5, -10, 1},
    {ITEM_REVIVAL_HERB, -15, -20, 401},
};

static void PrepareMedicineTarget(struct Pokemon *mon, enum Item item, u32 friendship)
{
    u32 maxHP = 401, hp = item == ITEM_REVIVAL_HERB ? 0 : 1;
    u32 status = item == ITEM_HEAL_POWDER ? STATUS1_PARALYSIS : STATUS1_NONE;
    SetMonData(mon, MON_DATA_MAX_HP, &maxHP);
    SetMonData(mon, MON_DATA_HP, &hp);
    SetMonData(mon, MON_DATA_STATUS, &status);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
}

TEST("Polished medicine uses source penalties across all happiness values without bonuses")
{
    u32 index = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sMedicine); i++)
        PARAMETRIZE { index = i; }
    SetConfig(CONFIG_POLISHED_ITEM_HEALING, TRUE);
    gMain.inBattle = FALSE;
    ZeroPlayerPartyMons();
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    CreateRandomMon(mon, SPECIES_CHANSEY, 50);
    u32 bell = ITEM_SOOTHE_BELL, ball = BALL_LUXURY, location = GetCurrentRegionMapSectionId();
    SetMonData(mon, MON_DATA_HELD_ITEM, &bell);
    SetMonData(mon, MON_DATA_POKEBALL, &ball);
    SetMonData(mon, MON_DATA_MET_LOCATION, &location);
    for (s32 friendship = 0; friendship <= MAX_FRIENDSHIP; friendship++)
    {
        PrepareMedicineTarget(mon, sMedicine[index].item, friendship);
        EXPECT(!PokemonUseItemEffects(mon, sMedicine[index].item, 0, 0, FALSE));
        s32 penalty = friendship < 160 ? sMedicine[index].low : sMedicine[index].high;
        EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), max(0, friendship + penalty));
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP), sMedicine[index].healedHP);
        EXPECT_EQ(GetMonData(mon, MON_DATA_STATUS), STATUS1_NONE);
    }
}

TEST("Polished medicine has no happiness penalty when the item has no effect")
{
    u32 index = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sMedicine); i++)
        PARAMETRIZE { index = i; }
    SetConfig(CONFIG_POLISHED_ITEM_HEALING, TRUE);
    gMain.inBattle = FALSE;
    ZeroPlayerPartyMons();
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    CreateRandomMon(mon, SPECIES_CHANSEY, 50);
    PrepareMedicineTarget(mon, sMedicine[index].item, 200);
    u32 hp = 401, status = STATUS1_NONE;
    SetMonData(mon, MON_DATA_HP, &hp);
    SetMonData(mon, MON_DATA_STATUS, &status);
    EXPECT(PokemonUseItemEffects(mon, sMedicine[index].item, 0, 0, FALSE));
    EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), 200);
}

TEST("Polished medicine preserves native penalties and optional Expansion medicine")
{
    u32 index = 0;
    for (u32 i = 0; i <= ARRAY_COUNT(sMedicine); i++)
        PARAMETRIZE { index = i; }
    gMain.inBattle = FALSE;
    ZeroPlayerPartyMons();
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    CreateRandomMon(mon, SPECIES_CHANSEY, 50);
    u32 location = GetCurrentRegionMapSectionId() ^ 1;
    SetMonData(mon, MON_DATA_MET_LOCATION, &location);
    enum Item item = index < ARRAY_COUNT(sMedicine) ? sMedicine[index].item : ITEM_REMEDY;
    for (u32 source = FALSE; source <= TRUE; source++)
    {
        SetConfig(CONFIG_POLISHED_ITEM_HEALING, source);
        PrepareMedicineTarget(mon, item, 170);
        EXPECT(!PokemonUseItemEffects(mon, item, 0, 0, FALSE));
        s32 penalty = -5;
        if (index < ARRAY_COUNT(sMedicine))
            penalty = source ? sMedicine[index].high : sMedicine[index].low;
        EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), 170 + penalty);
    }
}

TEST("Polished medicine happiness adapter excludes eggs and unmapped item IDs")
{
    SetConfig(CONFIG_POLISHED_ITEM_HEALING, TRUE);
    gMain.inBattle = FALSE;
    ZeroPlayerPartyMons();
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    CreateRandomMon(mon, SPECIES_CHANSEY, 50);
    u32 egg = TRUE, friendship = 170;
    SetMonData(mon, MON_DATA_IS_EGG, &egg);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
    EXPECT(PolishedApplyMedicineFriendship(mon, ITEM_ENERGY_ROOT));
    EXPECT_EQ(GetMonData(mon, MON_DATA_FRIENDSHIP), friendship);
    EXPECT(!PolishedApplyMedicineFriendship(mon, ITEM_POTION));
    EXPECT(!PolishedApplyMedicineFriendship(mon, ITEMS_COUNT));
    SetConfig(CONFIG_POLISHED_ITEM_HEALING, FALSE);
    EXPECT(!PolishedApplyMedicineFriendship(mon, ITEM_ENERGY_ROOT));
}
