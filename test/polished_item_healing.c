#include "global.h"
#include "battle.h"
#include "config_changes.h"
#include "item.h"
#include "item_use.h"
#include "polished_items.h"
#include "test/test.h"

static const struct { enum Item item; u16 hp; } sSourceHealing[] = {
    {ITEM_POTION, 20}, {ITEM_SUPER_POTION, 60}, {ITEM_HYPER_POTION, 120},
    {ITEM_MAX_POTION, 999}, {ITEM_FRESH_WATER, 30}, {ITEM_SODA_POP, 50},
    {ITEM_LEMONADE, 70}, {ITEM_MOOMOO_MILK, 100}, {ITEM_ENERGY_POWDER, 60},
    {ITEM_ENERGY_ROOT, 120}, {ITEM_ORAN_BERRY, 10}, {ITEM_BERRY_JUICE, 20},
    {ITEM_SITRUS_BERRY, 100}, {ITEM_FIGY_BERRY, 133}, // maxHP = 401
};

TEST("Polished healing uses all fourteen source amounts through the party item handler")
{
    u32 index = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sSourceHealing); i++)
        PARAMETRIZE { index = i; }
    SetConfig(CONFIG_POLISHED_ITEM_HEALING, TRUE);
    gMain.inBattle = FALSE;
    ZeroPlayerPartyMons();
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    CreateRandomMon(mon, SPECIES_CHANSEY, 50);
    enum Item item = sSourceHealing[index].item;
    u32 hp = 1, maxHP = 401;
    SetMonData(mon, MON_DATA_MAX_HP, &maxHP);
    SetMonData(mon, MON_DATA_HP, &hp);
    EXPECT_EQ(PolishedGetItemHealAmount(item, maxHP), sSourceHealing[index].hp);
    EXPECT(!PokemonUseItemEffects(mon, item, 0, 0, FALSE));
    EXPECT_EQ(GetMonData(mon, MON_DATA_HP), min(maxHP, 1 + sSourceHealing[index].hp));

    // Ordinary healing cannot revive or consume an item on a full-health mon.
    hp = 0;
    SetMonData(mon, MON_DATA_HP, &hp);
    EXPECT(PokemonUseItemEffects(mon, item, 0, 0, FALSE));
    EXPECT_EQ(GetMonData(mon, MON_DATA_HP), 0);
    SetMonData(mon, MON_DATA_HP, &maxHP);
    EXPECT(PokemonUseItemEffects(mon, item, 0, 0, FALSE));
    EXPECT_EQ(GetMonData(mon, MON_DATA_HP), maxHP);
}

TEST("Polished healing rounds source berry fractions down with a one HP minimum")
{
    SetConfig(CONFIG_POLISHED_ITEM_HEALING, TRUE);
    // The source GetThirdBC contract is HP < 768; cover its whole domain.
    for (u32 hp = 1; hp < 768; hp++)
    {
        EXPECT_EQ(PolishedGetItemHealAmount(ITEM_FIGY_BERRY, hp), max(1, hp / 3));
        EXPECT_EQ(PolishedGetItemHealAmount(ITEM_SITRUS_BERRY, hp), max(1, hp / 4));
    }
    // Preserve the GBA engine's capacity beyond the source's storage limit.
    EXPECT_EQ(PolishedGetItemHealAmount(ITEM_FIGY_BERRY, 1200), 400);
    EXPECT_EQ(PolishedGetItemHealAmount(ITEM_MAX_POTION, 1200), 999);
}

TEST("Polished healing enables Figy party and battle use only in the source profile")
{
    SetConfig(CONFIG_POLISHED_ITEM_HEALING, TRUE);
    EXPECT_EQ(GetItemType(ITEM_FIGY_BERRY), ITEM_USE_PARTY_MENU);
    EXPECT_EQ(GetItemFieldFunc(ITEM_FIGY_BERRY), ItemUseOutOfBattle_Medicine);
    EXPECT_EQ(GetItemBattleUsage(ITEM_FIGY_BERRY), EFFECT_ITEM_RESTORE_HP);
    EXPECT(GetItemEffect(ITEM_FIGY_BERRY) != NULL);
    EXPECT_EQ(PolishedGetItemHealAmount(ITEM_SWEET_HEART, 401), 0);
    EXPECT_EQ(PolishedGetItemHealAmount(ITEM_NONE, 401), 0);
    EXPECT_EQ(PolishedGetItemHealAmount(ITEMS_COUNT, 401), 0);
    SetConfig(CONFIG_POLISHED_ITEM_HEALING, FALSE);
    EXPECT_EQ(GetItemType(ITEM_FIGY_BERRY), gItemsInfo[ITEM_FIGY_BERRY].type);
    EXPECT_EQ(GetItemFieldFunc(ITEM_FIGY_BERRY), gItemsInfo[ITEM_FIGY_BERRY].fieldUseFunc);
    EXPECT_EQ(GetItemBattleUsage(ITEM_FIGY_BERRY), gItemsInfo[ITEM_FIGY_BERRY].battleUsage);
    EXPECT_EQ(GetItemEffect(ITEM_FIGY_BERRY), NULL);
    for (u32 i = 0; i < ARRAY_COUNT(sSourceHealing); i++)
        EXPECT_EQ(PolishedGetItemHealAmount(sSourceHealing[i].item, 401), 0);
}
