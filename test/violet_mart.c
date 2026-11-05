#include "global.h"
#include "item.h"
#include "item_use.h"
#include "mail.h"
#include "overworld.h"
#include "pokemon.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "test/test.h"

TEST("Violet Mart appends its door after the saved Academy and Center warps")
{
    const struct MapHeader *city = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_VIOLET_CITY), MAP_NUM(MAP_POLISHED_VIOLET_CITY));
    const struct MapHeader *mart = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_POLISHED_VIOLET_MART), MAP_NUM(MAP_POLISHED_VIOLET_MART));
    EXPECT_EQ(city->events->warps[3].mapNum, MAP_NUM(MAP_POLISHED_EARLS_POKEMON_ACADEMY));
    EXPECT_EQ(city->events->warps[4].mapNum, MAP_NUM(MAP_POLISHED_VIOLET_MART));
    EXPECT_EQ(city->events->warps[4].warpId, 1);
    EXPECT_EQ(mart->events->warpCount, 2);
    EXPECT_EQ(mart->events->warps[0].warpId, 4);
    EXPECT_EQ(mart->events->warps[1].warpId, 4);
}

TEST("Violet Mart Flower Mail uses native held mail and the PC mailbox")
{
    struct Pokemon mon;
    struct Mail original[MAIL_COUNT];
    u32 held, slot;
    memcpy(original, gSaveBlock1Ptr->mail, sizeof(original));
    ClearAllMail();
    CreateMon(&mon, SPECIES_PIDGEY, 5, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT(ItemIsMail(ITEM_POLISHED_FLOWER_MAIL));
    EXPECT(IS_ITEM_MAIL(ITEM_POLISHED_FLOWER_MAIL));
    EXPECT_EQ(ITEM_TO_MAIL(ITEM_POLISHED_FLOWER_MAIL), 12);
    EXPECT_EQ(ITEM_TO_MAIL(ITEM_ORANGE_MAIL), 0);
    EXPECT_EQ(ITEM_TO_MAIL(ITEM_RETRO_MAIL), 11);
    EXPECT(!ItemIsMail(ITEM_POLISHED_ESCAPE_ROPE));
    EXPECT_EQ(GiveMailToMonByItemId(&mon, ITEM_POLISHED_FLOWER_MAIL), 0);
    EXPECT(MonHasMail(&mon));
    held = GetMonData(&mon, MON_DATA_HELD_ITEM);
    EXPECT_EQ(held, ITEM_POLISHED_FLOWER_MAIL);
    slot = TakeMailFromMonAndSave(&mon);
    EXPECT_EQ(slot, PARTY_SIZE);
    EXPECT_EQ(gSaveBlock1Ptr->mail[slot].itemId, ITEM_POLISHED_FLOWER_MAIL);
    EXPECT(!MonHasMail(&mon));
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HELD_ITEM), ITEM_NONE);
    EXPECT_EQ(GiveMailToMonByItemId(&mon, ITEM_ORANGE_MAIL), 0);
    EXPECT(MonHasMail(&mon));
    memcpy(gSaveBlock1Ptr->mail, original, sizeof(original));
}

TEST("Violet Mart keeps the consumable Escape Rope and rejects full item pockets")
{
    struct ItemSlot original[BAG_ITEMS_COUNT];
    struct BagPocket *pocket = &gBagPockets[POCKET_ITEMS];
    memcpy(original, gSaveBlock1Ptr->bag.items, sizeof(original));
    EXPECT_EQ(GetItemPocket(ITEM_POLISHED_ESCAPE_ROPE), POCKET_ITEMS);
    EXPECT_EQ(GetItemImportance(ITEM_POLISHED_ESCAPE_ROPE), 0);
    EXPECT_EQ(GetItemPrice(ITEM_POLISHED_ESCAPE_ROPE), 1000);
    EXPECT(GetItemFieldFunc(ITEM_POLISHED_ESCAPE_ROPE) == ItemUseOutOfBattle_EscapeRope);
    EXPECT_EQ(GetItemPrice(ITEM_POLISHED_FLOWER_MAIL), 50);
    EXPECT(GetItemFieldFunc(ITEM_POLISHED_FLOWER_MAIL) == ItemUseOutOfBattle_Mail);
    for (u32 i = 0; i < ARRAY_COUNT(original); ++i)
        BagPocket_SetSlotItemIdAndCount(pocket, i, ITEM_POTION, MAX_BAG_ITEM_CAPACITY);
    EXPECT(!AddBagItem(ITEM_POLISHED_ESCAPE_ROPE, 1));
    EXPECT(!AddBagItem(ITEM_POLISHED_FLOWER_MAIL, 1));
    BagPocket_SetSlotItemIdAndCount(pocket, 0, ITEM_NONE, 0);
    EXPECT(AddBagItem(ITEM_POLISHED_FLOWER_MAIL, 1));
    EXPECT(CheckBagHasItem(ITEM_POLISHED_FLOWER_MAIL, 1));
    memcpy(gSaveBlock1Ptr->bag.items, original, sizeof(original));
}
