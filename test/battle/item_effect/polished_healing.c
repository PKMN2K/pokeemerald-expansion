#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polished healing applies source HP amounts to player and opponent")
{
    enum Item item;
    u32 hp;
    PARAMETRIZE { item = ITEM_POTION; hp = 20; }
    PARAMETRIZE { item = ITEM_SUPER_POTION; hp = 60; }
    PARAMETRIZE { item = ITEM_HYPER_POTION; hp = 120; }
    PARAMETRIZE { item = ITEM_MAX_POTION; hp = 400; }
    PARAMETRIZE { item = ITEM_FRESH_WATER; hp = 30; }
    PARAMETRIZE { item = ITEM_SODA_POP; hp = 50; }
    PARAMETRIZE { item = ITEM_LEMONADE; hp = 70; }
    PARAMETRIZE { item = ITEM_MOOMOO_MILK; hp = 100; }
    PARAMETRIZE { item = ITEM_ENERGY_POWDER; hp = 60; }
    PARAMETRIZE { item = ITEM_ENERGY_ROOT; hp = 120; }
    PARAMETRIZE { item = ITEM_ORAN_BERRY; hp = 10; }
    PARAMETRIZE { item = ITEM_BERRY_JUICE; hp = 20; }
    PARAMETRIZE { item = ITEM_SITRUS_BERRY; hp = 100; }
    PARAMETRIZE { item = ITEM_FIGY_BERRY; hp = 133; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); MaxHP(401); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(401); }
    } WHEN {
        TURN { USE_ITEM(player, item, partyIndex: 0); USE_ITEM(opponent, item, partyIndex: 0); }
    } SCENE {
        HP_BAR(player, damage: -hp);
        HP_BAR(opponent, damage: -hp);
    } THEN {
        EXPECT_EQ(player->hp, hp + 1);
        EXPECT_EQ(opponent->hp, hp + 1);
        EXPECT_EQ((u32)player->volatiles.confusionTimer, 0);
    }
}

SINGLE_BATTLE_TEST("Polished healing Figy use caps at full HP and heals benched Pokemon")
{
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { HP(399); MaxHP(401); }
        PLAYER(SPECIES_CHANSEY) { HP(1); MaxHP(401); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { USE_ITEM(player, ITEM_FIGY_BERRY, partyIndex: 0); }
        TURN { USE_ITEM(player, ITEM_FIGY_BERRY, partyIndex: 1); }
    } SCENE {
        HP_BAR(player, damage: -2);
    } THEN {
        EXPECT_EQ(player->hp, 401);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HP), 134);
    }
}

SINGLE_BATTLE_TEST("Polished healing berries restore at least one HP in battle")
{
    enum Item item;
    PARAMETRIZE { item = ITEM_SITRUS_BERRY; }
    PARAMETRIZE { item = ITEM_FIGY_BERRY; }
    GIVEN {
        WITH_CONFIG(POLISHED_ITEM_HEALING, TRUE);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); MaxHP(2); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { USE_ITEM(player, item, partyIndex: 0); }
    } SCENE {
        HP_BAR(player, damage: -1);
    } THEN {
        EXPECT_EQ(player->hp, 2);
    }
}
