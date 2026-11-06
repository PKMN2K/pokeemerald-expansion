#include "global.h"
#include "config_changes.h"
#include "decompress.h"
#include "dexnav.h"
#include "fieldmap.h"
#include "item.h"
#include "item_icon.h"
#include "palette.h"
#include "polished_items.h"
#include "polished_pokemon_data.h"
#include "pokemon_storage_system.h"
#include "save.h"
#include "sprite.h"
#include "string_util.h"
#include "battle.h"
#include "test/test.h"
#include "constants/rgb.h"
#include "constants/layouts.h"

static void EnableHeldItems(void)
{
    SetConfig(CONFIG_POLISHED_WILD_HELD_ITEMS, TRUE);
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    ZeroPlayerPartyMons();
    gBattleTypeFlags = 0;
    gDexNavSpecies = SPECIES_NONE;
}

static enum Item Generate(enum Species species, bool32 legendary)
{
    enum Item item = ITEM_MASTER_BALL;
    EXPECT(PolishedGenerateWildHeldItem(species, legendary, &item));
    return item;
}

TEST("Polished held items cover all source forms with empty rows and native fallback")
{
    EnableHeldItems();
    u32 count = 0;
    for (u32 species = 1; species < NUM_SPECIES; species++)
    {
        const struct PolishedWildHeldItems *data = PolishedGetWildHeldItems(species);
        EXPECT_EQ(data != NULL, PolishedGetSpeciesBaseData(species) != NULL);
        if (data != NULL)
        {
            EXPECT(data->common < ITEMS_COUNT && data->rare < ITEMS_COUNT);
            count++;
        }
    }
    EXPECT_EQ(count, 391);
    EXPECT_EQ(PolishedGetWildHeldItems(NUM_SPECIES), NULL);
    EXPECT_EQ(PolishedGetWildHeldItems(SPECIES_NONE), NULL);
    EXPECT_EQ(PolishedGetWildHeldItems(SPECIES_MAGIKARP_RAINDROP)->common, ITEM_NONE);
    EXPECT_EQ(PolishedGetWildHeldItems(SPECIES_MAGIKARP_RAINDROP)->rare, ITEM_NONE);
    EXPECT_EQ(PolishedGetWildHeldItems(SPECIES_MEWTWO_ARMORED)->rare, ITEM_PETAYA_BERRY);
    EXPECT_EQ(PolishedGetWildHeldItems(SPECIES_VULPIX_ALOLA)->rare, ITEM_ASPEAR_BERRY);
    EXPECT_EQ(PolishedGetWildHeldItems(SPECIES_FARFETCHD_GALAR)->rare, ITEM_LEEK);
    enum Item item = ITEM_MASTER_BALL;
    EXPECT(!PolishedGenerateWildHeldItem(SPECIES_MUDKIP, FALSE, &item));
    EXPECT_EQ(item, ITEM_MASTER_BALL);
    SetConfig(CONFIG_POLISHED_WILD_HELD_ITEMS, FALSE);
    EXPECT(!PolishedGenerateWildHeldItem(SPECIES_CHANSEY, FALSE, &item));
    EXPECT_EQ(item, ITEM_MASTER_BALL);
}

TEST("Polished held-item rolls use exact ordinary byte boundaries")
{
    u32 common = 0, rare = 0;
    enum Item expected = ITEM_NONE;
    PARAMETRIZE { common = 0; rare = 255; expected = ITEM_OVAL_STONE; }
    PARAMETRIZE { common = 126; rare = 255; expected = ITEM_OVAL_STONE; }
    PARAMETRIZE { common = 127; rare = 0; expected = ITEM_LUCKY_EGG; }
    PARAMETRIZE { common = 255; rare = 24; expected = ITEM_LUCKY_EGG; }
    PARAMETRIZE { common = 127; rare = 25; expected = ITEM_NONE; }
    PARAMETRIZE { common = 255; rare = 255; expected = ITEM_NONE; }
    EnableHeldItems();
    SET_RNG(RNG_POLISHED_HELD_ITEM_COMMON, common);
    SET_RNG(RNG_POLISHED_HELD_ITEM_RARE, rare);
    EXPECT_EQ(Generate(SPECIES_CHANSEY, FALSE), expected);
}

TEST("Polished held-item boost uses 153 and 127 byte thresholds")
{
    u32 common = 0, rare = 0;
    enum Item expected = ITEM_NONE;
    PARAMETRIZE { common = 127; rare = 255; expected = ITEM_OVAL_STONE; }
    PARAMETRIZE { common = 152; rare = 255; expected = ITEM_OVAL_STONE; }
    PARAMETRIZE { common = 153; rare = 25; expected = ITEM_LUCKY_EGG; }
    PARAMETRIZE { common = 255; rare = 126; expected = ITEM_LUCKY_EGG; }
    PARAMETRIZE { common = 153; rare = 127; expected = ITEM_NONE; }
    EnableHeldItems();
    CreateRandomMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_BUTTERFREE, 15);
    u32 ability = 0;
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_ABILITY_NUM, &ability);
    EXPECT_EQ(GetMonAbility(&gParties[B_TRAINER_PLAYER][0]), ABILITY_COMPOUND_EYES);
    SET_RNG(RNG_POLISHED_HELD_ITEM_COMMON, common);
    SET_RNG(RNG_POLISHED_HELD_ITEM_RARE, rare);
    EXPECT_EQ(Generate(SPECIES_CHANSEY, FALSE), expected);
}

TEST("Polished held-item boosts use physical lead abilities or Amulet Coin even when fainted")
{
    u32 kind = 0;
    for (u32 i = 0; i < 9; i++)
        PARAMETRIZE { kind = i; }
    EnableHeldItems();
    struct Pokemon *lead = &gParties[B_TRAINER_PLAYER][0];
    CreateRandomMon(lead, kind == 1 ? SPECIES_MURKROW : SPECIES_BUTTERFREE, 15);
    u32 ability = kind == 1 ? 1 : 0;
    SetMonData(lead, MON_DATA_ABILITY_NUM, &ability);
    if (kind == 1)
        EXPECT_EQ(GetMonAbility(lead), ABILITY_SUPER_LUCK);
    if (kind == 2)
    {
        u32 hp = 0;
        SetMonData(lead, MON_DATA_HP, &hp);
    }
    if (kind == 3 || kind == 5 || kind == 8)
    {
        u32 egg = TRUE;
        SetMonData(lead, MON_DATA_IS_EGG, &egg);
    }
    if (kind == 4 || kind == 5 || kind == 6 || kind == 8)
    {
        enum Item coin = ITEM_AMULET_COIN;
        SetMonData(lead, MON_DATA_HELD_ITEM, &coin);
    }
    if (kind == 4)
    {
        ability = 1; // Levitate: the item alone must provide the boost.
        SetMonData(lead, MON_DATA_ABILITY_NUM, &ability);
    }
    if (kind == 6 || kind == 7)
    {
        gParties[B_TRAINER_PLAYER][1] = *lead;
        CreateRandomMon(lead, SPECIES_TOTODILE, 15);
    }
    SET_RNG(RNG_POLISHED_HELD_ITEM_COMMON, 140);
    SET_RNG(RNG_POLISHED_HELD_ITEM_RARE, 255);
    EXPECT_EQ(Generate(SPECIES_CHANSEY, FALSE), kind == 3 || kind == 6 || kind == 7 ? ITEM_NONE : ITEM_OVAL_STONE);
    // Stacking ability and coin does not improve on the one boosted profile.
    SET_RNG(RNG_POLISHED_HELD_ITEM_COMMON, 153);
    SET_RNG(RNG_POLISHED_HELD_ITEM_RARE, 127);
    EXPECT_EQ(Generate(SPECIES_CHANSEY, FALSE), ITEM_NONE);
}

TEST("Polished guaranteed items precede legendary overrides and equal slots still roll")
{
    EnableHeldItems();
    SET_RNG(RNG_POLISHED_HELD_ITEM_COMMON, 255);
    SET_RNG(RNG_POLISHED_HELD_ITEM_RARE, 255);
    EXPECT_EQ(Generate(SPECIES_VULPIX, FALSE), ITEM_ASPEAR_BERRY);
    EXPECT_EQ(Generate(SPECIES_VULPIX, TRUE), ITEM_ASPEAR_BERRY);
    EXPECT_EQ(Generate(SPECIES_LAPRAS, TRUE), ITEM_MYSTIC_WATER);
    EXPECT_EQ(Generate(SPECIES_CHANSEY, TRUE), ITEM_OVAL_STONE);
    EXPECT_EQ(Generate(SPECIES_BULBASAUR, TRUE), ITEM_NONE);
    EXPECT_EQ(Generate(SPECIES_JIGGLYPUFF, FALSE), ITEM_NONE);
    EXPECT_EQ(Generate(SPECIES_JIGGLYPUFF, TRUE), ITEM_ORAN_BERRY);
    EXPECT_EQ(Generate(SPECIES_CHANSEY, FALSE), ITEM_NONE);
    gBattleTypeFlags = BATTLE_TYPE_ROAMER;
    EXPECT_EQ(Generate(SPECIES_CHANSEY, gBattleTypeFlags & BATTLE_TYPE_LEGENDARY), ITEM_NONE);
}

TEST("Polished item RNG consumes zero one or two draws including empty source rows")
{
    EnableHeldItems();
    // No rigging: compare the real generator state after the source's
    // branch-dependent draws, including NONE/NONE and equal-item rows.
    for (u32 seed = 0; seed < 16; seed++)
    {
        SeedRng(seed);
        rng_value_t start = gRngValue;
        EXPECT_EQ(Generate(SPECIES_SNORLAX, FALSE), ITEM_LEFTOVERS);
        EXPECT_EQ(memcmp(&start, &gRngValue, sizeof(start)), 0);
        EXPECT_EQ(Generate(SPECIES_CHANSEY, TRUE), ITEM_OVAL_STONE);
        EXPECT_EQ(memcmp(&start, &gRngValue, sizeof(start)), 0);
        if (RandomUniform(RNG_POLISHED_HELD_ITEM_COMMON, 0, 255) >= 127)
            RandomUniform(RNG_POLISHED_HELD_ITEM_RARE, 0, 255);
        rng_value_t after = gRngValue;
        SeedRng(seed);
        EXPECT_EQ(Generate(SPECIES_MAGIKARP, FALSE), ITEM_NONE);
        EXPECT_EQ(memcmp(&after, &gRngValue, sizeof(after)), 0);
        SeedRng(seed);
        Generate(SPECIES_JIGGLYPUFF, FALSE);
        EXPECT_EQ(memcmp(&after, &gRngValue, sizeof(after)), 0);
    }
}

TEST("Polished live wild-item assignment supports doubles explicit items and native isolation")
{
    EnableHeldItems();
    SET_RNG(RNG_POLISHED_HELD_ITEM_COMMON, 0);
    SET_RNG(RNG_POLISHED_HELD_ITEM_RARE, 255);
    struct Pokemon *first = &gParties[B_TRAINER_OPPONENT_A][0];
    struct Pokemon *second = &gParties[B_TRAINER_OPPONENT_A][1];
    CreateRandomMon(first, SPECIES_ODDISH, 15);
    CreateRandomMon(second, SPECIES_BELLSPROUT, 15);
    gBattleTypeFlags = BATTLE_TYPE_DOUBLE;
    SetWildMonHeldItem();
    EXPECT_EQ(GetMonData(first, MON_DATA_HELD_ITEM), ITEM_POLISHED_SILVER_LEAF);
    EXPECT_EQ(GetMonData(second, MON_DATA_HELD_ITEM), ITEM_POLISHED_GOLD_LEAF);
    enum Item explicitItem = ITEM_POLISHED_PINK_BOW;
    SetMonData(first, MON_DATA_HELD_ITEM, &explicitItem);
    SetWildMonHeldItem();
    EXPECT_EQ(GetMonData(first, MON_DATA_HELD_ITEM), explicitItem);
    CreateRandomMon(first, SPECIES_ODDISH, 15);
    gDexNavSpecies = SPECIES_ODDISH;
    SetWildMonHeldItem();
    EXPECT_EQ(GetMonData(first, MON_DATA_HELD_ITEM), ITEM_NONE);
    gDexNavSpecies = SPECIES_NONE;
    CreateRandomMon(first, SPECIES_MUDKIP, 15);
    gBattleTypeFlags = BATTLE_TYPE_LEGENDARY;
    SetWildMonHeldItem();
    EXPECT_EQ(GetMonData(first, MON_DATA_HELD_ITEM), ITEM_NONE);
    CreateRandomMon(first, SPECIES_CHANSEY, 15);
    SetWildMonHeldItem();
    EXPECT_EQ(GetMonData(first, MON_DATA_HELD_ITEM), ITEM_OVAL_STONE);
    u16 originalLayout = gMapHeader.mapLayoutId;
    gMapHeader.mapLayoutId = LAYOUT_ALTERING_CAVE;
    CreateRandomMon(first, SPECIES_TEDDIURSA, 15);
    gBattleTypeFlags = 0;
    SetWildMonHeldItem();
    EXPECT_EQ(GetMonData(first, MON_DATA_HELD_ITEM), ITEM_ORAN_BERRY);
    gMapHeader.mapLayoutId = originalLayout;
    SetConfig(CONFIG_POLISHED_WILD_HELD_ITEMS, FALSE);
    CreateRandomMon(first, SPECIES_SNORLAX, 15);
    gBattleTypeFlags = 0;
    SetWildMonHeldItem();
    EXPECT_EQ(GetMonData(first, MON_DATA_HELD_ITEM), ITEM_LEFTOVERS);
}

TEST("Polished captured held items survive party and PC flash save reload")
{
    enum Species species = SPECIES_NONE;
    enum Item item = ITEM_NONE;
    PARAMETRIZE { species = SPECIES_ODDISH; item = ITEM_POLISHED_SILVER_LEAF; }
    PARAMETRIZE { species = SPECIES_BELLSPROUT; item = ITEM_POLISHED_GOLD_LEAF; }
    PARAMETRIZE { species = SPECIES_SYLVEON; item = ITEM_POLISHED_PINK_BOW; }
    EnableHeldItems();
    struct Pokemon *wild = &gParties[B_TRAINER_OPPONENT_A][0];
    CreateRandomMon(wild, species, 15);
    SET_RNG(RNG_POLISHED_HELD_ITEM_COMMON, 255);
    SET_RNG(RNG_POLISHED_HELD_ITEM_RARE, 0);
    if (species != SPECIES_SYLVEON)
        SET_RNG(RNG_POLISHED_HELD_ITEM_COMMON, 0);
    SetWildMonHeldItem();
    EXPECT_EQ(GetMonData(wild, MON_DATA_HELD_ITEM), item);
    EXPECT_EQ(GiveCapturedMonToPlayer(wild), MON_GIVEN_TO_PARTY);
    struct Pokemon *caught = &gParties[B_TRAINER_PLAYER][0];
    struct BoxPokemon expected = caught->box;
    SetBoxMonAt(0, 0, &caught->box);
    Save_ResetSaveCounters();
    EXPECT_EQ(TrySavingData(SAVE_NORMAL), SAVE_STATUS_OK);
    ZeroPlayerPartyMons();
    ZeroBoxMonAt(0, 0);
    EXPECT_EQ(LoadGameSave(SAVE_NORMAL), SAVE_STATUS_OK);
    EXPECT_EQ(GetMonData(caught, MON_DATA_HELD_ITEM), item);
    EXPECT_EQ(GetBoxMonData(GetBoxedMonPtr(0, 0), MON_DATA_HELD_ITEM), item);
    EXPECT_EQ(memcmp(&expected, &caught->box, sizeof(expected)), 0);
    EXPECT_EQ(memcmp(&expected, GetBoxedMonPtr(0, 0), sizeof(expected)), 0);
    EXPECT_EQ(GetMonData(caught, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
}

TEST("Polished item icons load original tiles and palettes through the native sprite path")
{
    enum Item item = ITEM_NONE;
    u32 hash = 0;
    u16 light = 0, dark = 0;
    PARAMETRIZE { item = ITEM_POLISHED_PINK_BOW; hash = 0x2cc63652; light = RGB(31, 18, 23); dark = RGB(28, 12, 17); }
    PARAMETRIZE { item = ITEM_POLISHED_SILVER_LEAF; hash = 0x6d392838; light = RGB(19, 24, 22); dark = RGB(12, 16, 15); }
    PARAMETRIZE { item = ITEM_POLISHED_GOLD_LEAF; hash = 0x6d392838; light = RGB(31, 25, 8); dark = RGB(27, 18, 1); }
    u32 aligned[0x120 / sizeof(u32)];
    u8 *tiles = (u8 *)aligned;
    DecompressDataWithHeaderWram(GetItemIconPic(item), tiles);
    u32 actual = 2166136261u;
    for (u32 i = 0; i < sizeof(aligned); i++)
        actual = (actual ^ tiles[i]) * 16777619u;
    EXPECT_EQ(actual, hash);
    EXPECT_EQ(GetItemPrice(item), item == ITEM_POLISHED_PINK_BOW ? 1000 : 500);
    EXPECT_EQ(GetItemPocket(item), POCKET_ITEMS);
    u8 name[ITEM_NAME_PLURAL_LENGTH + 1];
    CopyItemNameHandlePlural(item, name, 2);
    EXPECT_EQ(StringCompare(name, item == ITEM_POLISHED_PINK_BOW ? COMPOUND_STRING("Pink Bows")
                                : item == ITEM_POLISHED_SILVER_LEAF ? COMPOUND_STRING("Silver Leaves")
                                : COMPOUND_STRING("Gold Leaves")), 0);
    ResetSpriteData();
    FreeAllSpritePalettes();
    u8 spriteId = AddItemIconSprite(1000, 1000, item);
    EXPECT(spriteId < MAX_SPRITES);
    u16 paletteIndex = 256 + gSprites[spriteId].oam.paletteNum * 16;
    EXPECT_EQ(gPlttBufferUnfaded[paletteIndex], RGB_WHITE);
    EXPECT_EQ(gPlttBufferUnfaded[paletteIndex + 1], light);
    EXPECT_EQ(gPlttBufferUnfaded[paletteIndex + 2], dark);
    EXPECT_EQ(gPlttBufferUnfaded[paletteIndex + 3], RGB_BLACK);
    DestroySpriteAndFreeResources(&gSprites[spriteId]);
}
