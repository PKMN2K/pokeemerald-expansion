#include "global.h"
#include "config_changes.h"
#include "daycare.h"
#include "egg_hatch.h"
#include "event_data.h"
#include "move.h"
#include "polished_pokemon_data.h"
#include "pokemon_storage_system.h"
#include "save.h"
#include "test/test.h"

static void EnableLearning(void)
{
    SetConfig(CONFIG_POLISHED_LEARNING_DATA, TRUE);
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, TRUE);
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, TRUE);
    SetConfig(CONFIG_POLISHED_FORMS, TRUE);
}

static void Parent(u32 slot, enum Species species, u8 gender, const enum Move *moves)
{
    struct BoxPokemon *parent = &gSaveBlock1Ptr->daycare.mons[slot].mon;
    CreateBoxMon(parent, species, 50, GetMonPersonality(species, gender, NATURE_HARDY, RANDOM_UNOWN_LETTER), OTID_STRUCT_PLAYER_ID);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        SetBoxMonData(parent, MON_DATA_MOVE1 + i, &moves[i]);
}

static void ExpectMoves(struct Pokemon *mon, const enum Move *moves)
{
    EXPECT_EQ(GetMonData(mon, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        EXPECT_EQ(GetMonData(mon, MON_DATA_MOVE1 + i), moves[i]);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP1 + i), GetMovePP(moves[i]));
    }
    EXPECT_EQ(GetMonData(mon, MON_DATA_PP_BONUSES), 0);
}

TEST("Polished learning covers all source forms with explicit empty lists and native fallback")
{
    EnableLearning();
    u32 count = 0;
    for (u32 species = 1; species < NUM_SPECIES; species++)
    {
        if (PolishedGetLevelUpLearnset(species) == NULL)
            continue;
        EXPECT(PolishedGetEggMoves(species) != NULL);
        EXPECT(PolishedGetTeachableLearnset(species) != NULL);
        EXPECT_EQ(GetSpeciesEggMoves(species), PolishedGetEggMoves(species));
        EXPECT_EQ(GetSpeciesTeachableLearnset(species), PolishedGetTeachableLearnset(species));
        count++;
    }
    EXPECT_EQ(count, 391);
    EXPECT_EQ(GetSpeciesEggMoves(SPECIES_MAGIKARP_RAINDROP)[0], MOVE_UNAVAILABLE);
    EXPECT_EQ(GetSpeciesTeachableLearnset(SPECIES_MAGIKARP)[0], MOVE_UNAVAILABLE);
    EXPECT(CanLearnTeachableMove(SPECIES_TOTODILE, MOVE_DYNAMICPUNCH));
    EXPECT(CanLearnTeachableMove(SPECIES_TOTODILE, MOVE_SURF));
    EXPECT(CanLearnTeachableMove(SPECIES_TOTODILE, MOVE_ICE_PUNCH));
    EXPECT(!CanLearnTeachableMove(SPECIES_TOTODILE, MOVE_TERA_BLAST));
    EXPECT(SpeciesHasEggMove(SPECIES_SLOWPOKE, MOVE_SLEEP_POWDER));
    EXPECT(SpeciesHasEggMove(SPECIES_SLOWPOKE, MOVE_DOUBLE_SLAP));
    EXPECT(!SpeciesHasEggMove(SPECIES_SLOWPOKE_GALAR, MOVE_SLEEP_POWDER));
    EXPECT(SpeciesHasEggMove(SPECIES_VULPIX_ALOLA, MOVE_ICE_BEAM));
    EXPECT(!SpeciesHasEggMove(SPECIES_VULPIX, MOVE_ICE_BEAM));
    EXPECT_EQ(GetSpeciesEggMoves(SPECIES_MUDKIP), gSpeciesInfo[SPECIES_MUDKIP].eggMoveLearnset);
    EXPECT_EQ(GetSpeciesTeachableLearnset(SPECIES_MUDKIP), gSpeciesInfo[SPECIES_MUDKIP].teachableLearnset);
    SetConfig(CONFIG_POLISHED_LEARNING_DATA, FALSE);
    EXPECT_EQ(GetSpeciesEggMoves(SPECIES_TOTODILE), gSpeciesInfo[SPECIES_TOTODILE].eggMoveLearnset);
    EXPECT_EQ(GetSpeciesTeachableLearnset(SPECIES_TOTODILE), gSpeciesInfo[SPECIES_TOTODILE].teachableLearnset);
}

TEST("Polished daycare inherits shared moves in physical slot order before either parent's egg moves")
{
    u32 motherSlot = 0;
    PARAMETRIZE { motherSlot = 0; }
    PARAMETRIZE { motherSlot = 1; }
    EnableLearning();
    ZeroPlayerPartyMons();
    memset(&gSaveBlock1Ptr->daycare, 0, sizeof(struct DayCare));
    const enum Move first[] = {MOVE_AGILITY, MOVE_SLASH, MOVE_CRUNCH, MOVE_HYDRO_PUMP};
    const enum Move second[] = {MOVE_HYDRO_PUMP, MOVE_CRUNCH, MOVE_SLASH, MOVE_AGILITY};
    Parent(0, SPECIES_TOTODILE, motherSlot == 0 ? MON_FEMALE : MON_MALE, first);
    Parent(1, SPECIES_TOTODILE, motherSlot == 1 ? MON_FEMALE : MON_MALE, second);
    GiveEggFromDaycare();
    ExpectMoves(&gParties[B_TRAINER_PLAYER][0], first);
}

TEST("Polished daycare gives father then mother egg moves with FIFO overflow and no duplicate moves")
{
    u32 motherSlot = 0;
    PARAMETRIZE { motherSlot = 0; }
    PARAMETRIZE { motherSlot = 1; }
    EnableLearning();
    ZeroPlayerPartyMons();
    memset(&gSaveBlock1Ptr->daycare, 0, sizeof(struct DayCare));
    const enum Move father[] = {MOVE_DRAGON_DANCE, MOVE_ANCIENTPOWER, MOVE_CRUNCH, MOVE_ROCK_SLIDE};
    const enum Move mother[] = {MOVE_CRUNCH, MOVE_AQUA_JET, MOVE_WATER_PULSE, MOVE_TERA_BLAST};
    // Shared Crunch is already present when each parent offers it. Later
    // overflow removes it; the source does not promote an earlier duplicate.
    const enum Move expected[] = {MOVE_ANCIENTPOWER, MOVE_ROCK_SLIDE, MOVE_AQUA_JET, MOVE_WATER_PULSE};
    Parent(motherSlot ^ 1, SPECIES_TOTODILE, MON_MALE, father);
    Parent(motherSlot, SPECIES_TOTODILE, MON_FEMALE, mother);
    GiveEggFromDaycare();
    ExpectMoves(&gParties[B_TRAINER_PLAYER][0], expected);
}

TEST("Polished daycare uses the non-Ditto parent last regardless of gender or physical slot")
{
    u32 slot = 0;
    u8 gender = MON_MALE;
    PARAMETRIZE { slot = 0; gender = MON_MALE; }
    PARAMETRIZE { slot = 1; gender = MON_MALE; }
    PARAMETRIZE { slot = 0; gender = MON_FEMALE; }
    PARAMETRIZE { slot = 1; gender = MON_FEMALE; }
    EnableLearning();
    ZeroPlayerPartyMons();
    memset(&gSaveBlock1Ptr->daycare, 0, sizeof(struct DayCare));
    const enum Move ditto[] = {MOVE_DRAGON_DANCE, MOVE_ANCIENTPOWER, MOVE_CRUNCH, MOVE_ROCK_SLIDE};
    const enum Move parent[] = {MOVE_AQUA_JET, MOVE_WATER_PULSE, MOVE_METAL_CLAW, MOVE_THRASH};
    Parent(slot ^ 1, SPECIES_DITTO, MON_GENDERLESS, ditto);
    Parent(slot, SPECIES_TOTODILE, gender, parent);
    GiveEggFromDaycare();
    ExpectMoves(&gParties[B_TRAINER_PLAYER][0], parent);
}

TEST("Polished daycare rejects TM inheritance and Light Ball Volt Tackle injection")
{
    EnableLearning();
    ZeroPlayerPartyMons();
    memset(&gSaveBlock1Ptr->daycare, 0, sizeof(struct DayCare));
    const enum Move moves[] = {MOVE_THUNDERBOLT, MOVE_SURF, MOVE_VOLT_TACKLE, MOVE_NONE};
    const enum Move expected[] = {MOVE_THUNDERSHOCK, MOVE_CHARM, MOVE_NONE, MOVE_NONE};
    Parent(0, SPECIES_PIKACHU, MON_FEMALE, moves);
    Parent(1, SPECIES_PIKACHU, MON_MALE, moves);
    enum Item item = ITEM_LIGHT_BALL;
    SetBoxMonData(&gSaveBlock1Ptr->daycare.mons[0].mon, MON_DATA_HELD_ITEM, &item);
    GiveEggFromDaycare();
    ExpectMoves(&gParties[B_TRAINER_PLAYER][0], expected);
}

TEST("Polished traded eggs retain encrypted identity and shiny status through hatching")
{
    u32 order = 0;
    bool32 shiny = FALSE;
    for (u32 i = 0; i < 24; i++)
    {
        PARAMETRIZE { order = i; shiny = FALSE; }
        PARAMETRIZE { order = i; shiny = TRUE; }
    }
    EnableLearning();
    ZeroPlayerPartyMons();
    struct Pokemon *egg = &gParties[B_TRAINER_PLAYER][0];
    u32 foreignOt = READ_OTID_FROM_SAVE ^ 0x12345678;
    CreateMon(egg, SPECIES_MAGIKARP, 1, order, OTID_STRUCT_PRESET(foreignOt));
    u32 isEgg = TRUE, form = 20, ivs = 0x12345678;
    SetMonData(egg, MON_DATA_IS_EGG, &isEgg);
    SetMonData(egg, MON_DATA_POLISHED_FORM, &form);
    SetMonData(egg, MON_DATA_IVS, &ivs);
    SetMonData(egg, MON_DATA_IS_SHINY, &shiny);
    SetMonMoveSlot(egg, MOVE_REVERSAL, 0);
    CalculatePlayerPartyCount();
    gSpecialVar_0x8004 = 0;
    ScriptHatchMon();
    EXPECT_EQ(GetMonData(egg, MON_DATA_SPECIES), SPECIES_MAGIKARP);
    EXPECT_EQ(GetMonData(egg, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
    EXPECT_EQ(GetMonData(egg, MON_DATA_OT_ID), READ_OTID_FROM_SAVE);
    EXPECT_EQ(GetMonData(egg, MON_DATA_PERSONALITY), order);
    EXPECT_EQ(GetMonData(egg, MON_DATA_POLISHED_FORM), form);
    EXPECT_EQ(GetMonData(egg, MON_DATA_IS_SHINY), shiny);
    EXPECT_EQ(GetMonData(egg, MON_DATA_IVS), ivs);
    EXPECT_EQ(GetMonData(egg, MON_DATA_MOVE1), MOVE_REVERSAL);
    EXPECT_EQ(GetMonData(egg, MON_DATA_PP1), GetMovePP(MOVE_REVERSAL));
    EXPECT_EQ(GetMonData(egg, MON_DATA_IS_EGG), FALSE);
    EXPECT_EQ(GetMonData(egg, MON_DATA_FRIENDSHIP), 120);
}

TEST("Polished learning leaves native shared-level inheritance working beyond the first learnset row")
{
    SetConfig(CONFIG_POLISHED_LEARNING_DATA, FALSE);
    SetConfig(CONFIG_POLISHED_LEVEL_UP_LEARNSETS, FALSE);
    ZeroPlayerPartyMons();
    memset(&gSaveBlock1Ptr->daycare, 0, sizeof(struct DayCare));
    const enum Move moves[] = {MOVE_TACKLE, MOVE_NONE, MOVE_NONE, MOVE_NONE};
    const enum Move expected[] = {MOVE_SPLASH, MOVE_TACKLE, MOVE_NONE, MOVE_NONE};
    Parent(0, SPECIES_MAGIKARP, MON_MALE, moves);
    Parent(1, SPECIES_MAGIKARP, MON_FEMALE, moves);
    GiveEggFromDaycare();
    ExpectMoves(&gParties[B_TRAINER_PLAYER][0], expected);
}

TEST("Polished traded egg identity survives hatching then party and PC flash save reload")
{
    EnableLearning();
    ZeroPlayerPartyMons();
    struct Pokemon *egg = &gParties[B_TRAINER_PLAYER][0];
    CreateMon(egg, SPECIES_GYARADOS_RED, 1, 23, OTID_STRUCT_PRESET(READ_OTID_FROM_SAVE ^ 0x87654321));
    u32 isEgg = TRUE, shiny = TRUE;
    SetMonData(egg, MON_DATA_IS_EGG, &isEgg);
    SetMonData(egg, MON_DATA_IS_SHINY, &shiny);
    CalculatePlayerPartyCount();
    gSpecialVar_0x8004 = 0;
    ScriptHatchMon();
    EXPECT_EQ(GetMonData(egg, MON_DATA_SPECIES), SPECIES_GYARADOS_RED);
    struct BoxPokemon expected = egg->box;
    SetBoxMonAt(0, 0, &egg->box);
    Save_ResetSaveCounters();
    EXPECT_EQ(TrySavingData(SAVE_NORMAL), SAVE_STATUS_OK);
    ZeroPlayerPartyMons();
    ZeroBoxMonAt(0, 0);
    EXPECT_EQ(LoadGameSave(SAVE_NORMAL), SAVE_STATUS_OK);
    EXPECT_EQ(memcmp(&expected, &egg->box, sizeof(expected)), 0);
    EXPECT_EQ(memcmp(&expected, GetBoxedMonPtr(0, 0), sizeof(expected)), 0);
    EXPECT_EQ(GetMonData(egg, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
    EXPECT_EQ(GetMonData(egg, MON_DATA_POLISHED_FORM), 21);
    EXPECT_EQ(GetMonData(egg, MON_DATA_IS_SHINY), TRUE);
}

TEST("Polished ownership changes preserve all encrypted bytes and round trip every storage order")
{
    u32 order = 0;
    bool32 shiny = FALSE;
    for (u32 i = 0; i < 24; i++)
    {
        PARAMETRIZE { order = i; shiny = FALSE; }
        PARAMETRIZE { order = i; shiny = TRUE; }
    }
    struct BoxPokemon mon;
    CreateBoxMon(&mon, SPECIES_MUDKIP, 35, order, OTID_STRUCT_PRESET(order));
    SetBoxMonData(&mon, MON_DATA_IS_SHINY, &shiny);
    struct BoxPokemon before = mon;
    u32 otId = order ^ 0x8000;
    SetBoxMonData(&mon, MON_DATA_OT_ID, &otId);
    EXPECT_EQ(mon.checksum, before.checksum);
    EXPECT_EQ(mon.personality, before.personality);
    for (u32 i = 0; i < ARRAY_COUNT(mon.secure.raw); i++)
        EXPECT_EQ(mon.secure.raw[i] ^ otId, before.secure.raw[i] ^ before.otId);
    EXPECT_EQ(GetBoxMonData(&mon, MON_DATA_IS_SHINY), shiny);
    EXPECT_EQ(GetBoxMonData(&mon, MON_DATA_SPECIES), SPECIES_MUDKIP);
    SetBoxMonData(&mon, MON_DATA_OT_ID, &before.otId);
    EXPECT_EQ(memcmp(&mon, &before, sizeof(mon)), 0);
    // Rekeying cannot repair corrupt data or bless it with a new checksum.
    mon.secure.raw[0] ^= 1;
    SetBoxMonData(&mon, MON_DATA_OT_ID, &otId);
    EXPECT_EQ(mon.checksum, before.checksum);
    GetBoxMonData(&mon, MON_DATA_SPECIES);
    EXPECT_EQ(GetBoxMonData(&mon, MON_DATA_SANITY_IS_BAD_EGG), TRUE);
}
