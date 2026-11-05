#include "global.h"
#include "battle_setup.h"
#include "event_data.h"
#include "item.h"
#include "malloc.h"
#include "party_menu.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "polished_falkner.h"
#include "test/test.h"

TEST("Falkner TM51 teaches Roost and preserves existing HM item values")
{
    EXPECT_EQ(ITEM_TM_ROOST, 632);
    EXPECT_EQ(ITEM_HM_CUT, 682);
    EXPECT_EQ(ITEM_HM_FLASH, 686);
    EXPECT_EQ(ItemIdToBattleMoveId(ITEM_TM_ROOST), MOVE_ROOST);
    EXPECT_EQ(ItemIdToBattleMoveId(ITEM_HM_FLASH), MOVE_FLASH);
    EXPECT(GetItemImportance(ITEM_TM_ROOST));
    EXPECT(CanLearnTeachableMove(SPECIES_PIDGEOTTO, MOVE_ROOST));
    EXPECT(!CanLearnTeachableMove(SPECIES_TOTODILE, MOVE_ROOST));
}

TEST("Falkner badge is idempotent and updates only the first earned badge bit")
{
    u16 original = VarGet(VAR_POLISHED_EARNED_BADGES);
    bool32 zephyr = FlagGet(FLAG_POLISHED_ZEPHYR_BADGE);
    bool32 first = FlagGet(FLAG_BADGE01_GET);
    VarSet(VAR_POLISHED_EARNED_BADGES, 0x8040);
    PolishedFalknerAwardBadge();
    EXPECT_EQ(VarGet(VAR_POLISHED_EARNED_BADGES), 0x8041);
    EXPECT(FlagGet(FLAG_POLISHED_ZEPHYR_BADGE));
    EXPECT(FlagGet(FLAG_BADGE01_GET));
    PolishedFalknerAwardBadge();
    EXPECT_EQ(VarGet(VAR_POLISHED_EARNED_BADGES), 0x8041);
    VarSet(VAR_POLISHED_EARNED_BADGES, original);
    if (!zephyr) FlagClear(FLAG_POLISHED_ZEPHYR_BADGE);
    if (!first) FlagClear(FLAG_BADGE01_GET);
}

TEST("Togepi Egg delivery guards progress, uses party or PC, retries full storage and cannot duplicate")
{
    struct Pokemon original[PARTY_SIZE];
    u8 originalCount = gPartiesCount[B_TRAINER_PLAYER];
    struct PokemonStorage *originalStorage = gPokemonStoragePtr;
    struct PokemonStorage *storage = AllocZeroed(sizeof(*storage));
    const u16 flags[] = {FLAG_POLISHED_EGG_CALL, FLAG_POLISHED_ZEPHYR_BADGE,
        FLAG_POLISHED_EGG_GIVEN, FLAG_POLISHED_AIDE_IN_VIOLET};
    bool32 originalFlags[ARRAY_COUNT(flags)];
    u16 originalBox = VarGet(VAR_PC_BOX_TO_SEND_MON);
    EXPECT(storage != NULL);
    if (storage == NULL) return;
    memcpy(original, gParties[B_TRAINER_PLAYER], sizeof(original));
    gPokemonStoragePtr = storage;
    for (u32 i = 0; i < ARRAY_COUNT(flags); i++)
    {
        originalFlags[i] = FlagGet(flags[i]);
        FlagClear(flags[i]);
    }
    ZeroPlayerPartyMons();
    gPartiesCount[B_TRAINER_PLAYER] = 0;
    EXPECT_EQ(PolishedFalknerGiveEgg(), MON_CANT_GIVE);
    FlagSet(FLAG_POLISHED_ZEPHYR_BADGE);
    EXPECT_EQ(PolishedFalknerGiveEgg(), MON_CANT_GIVE);
    FlagSet(FLAG_POLISHED_EGG_CALL);
    FlagSet(FLAG_POLISHED_AIDE_IN_VIOLET);
    EXPECT_EQ(PolishedFalknerGiveEgg(), MON_GIVEN_TO_PARTY);
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_TOGEPI);
    EXPECT(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_IS_EGG));
    EXPECT(FlagGet(FLAG_POLISHED_EGG_GIVEN));
    EXPECT(!FlagGet(FLAG_POLISHED_AIDE_IN_VIOLET));
    EXPECT_EQ(PolishedFalknerGiveEgg(), MON_CANT_GIVE);
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 1);

    FlagClear(FLAG_POLISHED_EGG_GIVEN);
    FlagSet(FLAG_POLISHED_AIDE_IN_VIOLET);
    for (u32 i = 0; i < PARTY_SIZE; i++)
        CreateRandomMon(&gParties[B_TRAINER_PLAYER][i], SPECIES_PIDGEY, 5);
    gPartiesCount[B_TRAINER_PLAYER] = PARTY_SIZE;
    EXPECT_EQ(PolishedFalknerGiveEgg(), MON_GIVEN_TO_PC);
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], PARTY_SIZE);
    EXPECT_EQ(GetBoxMonData(&storage->boxes[0][0], MON_DATA_SPECIES), SPECIES_TOGEPI);
    EXPECT(GetBoxMonData(&storage->boxes[0][0], MON_DATA_IS_EGG));
    FlagClear(FLAG_POLISHED_EGG_GIVEN);
    FlagSet(FLAG_POLISHED_AIDE_IN_VIOLET);
    for (u32 box = 0; box < TOTAL_BOXES_COUNT; box++)
        for (u32 slot = 0; slot < IN_BOX_COUNT; slot++)
            storage->boxes[box][slot] = gParties[B_TRAINER_PLAYER][0].box;
    EXPECT_EQ(PolishedFalknerGiveEgg(), MON_CANT_GIVE);
    EXPECT(!FlagGet(FLAG_POLISHED_EGG_GIVEN));
    EXPECT(FlagGet(FLAG_POLISHED_AIDE_IN_VIOLET));
    ZeroBoxMonData(&storage->boxes[2][3]);
    EXPECT_EQ(PolishedFalknerGiveEgg(), MON_GIVEN_TO_PC);
    EXPECT_EQ(GetBoxMonData(&storage->boxes[2][3], MON_DATA_SPECIES), SPECIES_TOGEPI);
    EXPECT(GetBoxMonData(&storage->boxes[2][3], MON_DATA_IS_EGG));
    EXPECT_EQ(PolishedFalknerGiveEgg(), MON_CANT_GIVE);

    gPokemonStoragePtr = originalStorage;
    Free(storage);
    memcpy(gParties[B_TRAINER_PLAYER], original, sizeof(original));
    gPartiesCount[B_TRAINER_PLAYER] = originalCount;
    VarSet(VAR_PC_BOX_TO_SEND_MON, originalBox);
    for (u32 i = 0; i < ARRAY_COUNT(flags); i++)
        if (originalFlags[i]) FlagSet(flags[i]); else FlagClear(flags[i]);
}

#if !IS_FRLG
TEST("Violet Gym trainer flags preserve existing trainer and story IDs")
{
    const u16 trainers[] = {TRAINER_POLISHED_ROD, TRAINER_POLISHED_ABE, TRAINER_POLISHED_FALKNER};
    const u16 flags[] = {FLAG_POLISHED_ROD, FLAG_POLISHED_ABE, FLAG_POLISHED_FALKNER};
    for (u32 i = 0; i < ARRAY_COUNT(trainers); i++)
    {
        EXPECT(IsRegularTrainerId(trainers[i]));
        EXPECT(!IsPartnerTrainerId(trainers[i]));
        EXPECT_EQ(GetTrainerFlagId(trainers[i]), flags[i]);
    }
    EXPECT_EQ(GetTrainerFlagId(TRAINER_POLISHED_SPROUT_LI), 0x4e5);
    EXPECT_EQ(GetTrainerFlagId(TRAINER_PARTNER(PARTNER_STEVEN)), 0);
    EXPECT_EQ(FLAG_DEFEATED_RUSTBORO_GYM, 0x4f0);
    EXPECT_EQ(FLAG_DEFEATED_PETALBURG_GYM, 0x4f4);
}
#endif
