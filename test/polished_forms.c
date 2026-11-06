#include "global.h"
#include "config_changes.h"
#include "daycare.h"
#include "egg_hatch.h"
#include "event_data.h"
#include "polished_evolution.h"
#include "polished_forms.h"
#include "pokemon_storage_system.h"
#include "script_pokemon_util.h"
#include "save.h"
#include "wild_encounter.h"
#include "test/test.h"

static const enum Species sMagikarpPatterns[] = {
    SPECIES_MAGIKARP, SPECIES_MAGIKARP_SKELLY, SPECIES_MAGIKARP_CALICO1,
    SPECIES_MAGIKARP_CALICO2, SPECIES_MAGIKARP_CALICO3, SPECIES_MAGIKARP_TWO_TONE,
    SPECIES_MAGIKARP_ORCA, SPECIES_MAGIKARP_DAPPLES, SPECIES_MAGIKARP_TIGER,
    SPECIES_MAGIKARP_ZEBRA, SPECIES_MAGIKARP_STRIPE, SPECIES_MAGIKARP_BUBBLES,
    SPECIES_MAGIKARP_DIAMONDS, SPECIES_MAGIKARP_PATCHES, SPECIES_MAGIKARP_FOREHEAD1,
    SPECIES_MAGIKARP_MASK1, SPECIES_MAGIKARP_FOREHEAD2, SPECIES_MAGIKARP_MASK2,
    SPECIES_MAGIKARP_SAUCY, SPECIES_MAGIKARP_RAINDROP,
};

static void EnableForms(void)
{
    SetConfig(CONFIG_POLISHED_FORMS, TRUE);
    SetConfig(CONFIG_POLISHED_EVOLUTIONS, TRUE);
    SetConfig(CONFIG_POLISHED_SPECIES_DATA, TRUE);
}

TEST("Polished wild Magikarp select every source pattern before moves and stats")
{
    u32 form = 1;
    for (u32 i = 1; i <= 20; i++)
        PARAMETRIZE { form = i; }
    EnableForms();
    SET_RNG(RNG_POLISHED_WILD_FORM, form);
    CreateWildMon(SPECIES_MAGIKARP, 20);
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
    EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), sMagikarpPatterns[form - 1]);
    EXPECT_EQ(PolishedGetBoxMonForm(&mon->box), form);
    EXPECT_EQ(GetMonData(mon, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
    ZeroPlayerPartyMons();
    EXPECT_EQ(GiveCapturedMonToPlayer(mon), MON_GIVEN_TO_PARTY);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), sMagikarpPatterns[form - 1]);
    EXPECT_EQ(PolishedGetBoxMonForm(&gParties[B_TRAINER_PLAYER][0].box), form);
}

TEST("Polished wild Ekans and Arbok choose the two ordinary source forms equally")
{
    enum Species species = SPECIES_NONE;
    u32 form = 1;
    PARAMETRIZE { species = SPECIES_EKANS; form = 1; }
    PARAMETRIZE { species = SPECIES_EKANS; form = 2; }
    PARAMETRIZE { species = SPECIES_ARBOK; form = 1; }
    PARAMETRIZE { species = SPECIES_ARBOK; form = 2; }
    EnableForms();
    SET_RNG(RNG_POLISHED_WILD_FORM, form);
    CreateWildMon(species, 22);
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
    EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), species == SPECIES_ARBOK && form == 2 ? SPECIES_ARBOK_KANTO : species);
    EXPECT_EQ(PolishedGetBoxMonForm(&mon->box), form);
    if (species == SPECIES_EKANS)
        EXPECT_EQ(GetEvolutionTargetSpecies(mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), form == 1 ? SPECIES_ARBOK : SPECIES_ARBOK_KANTO);
}

TEST("Polished wild Dunsparce and Dudunsparce have one three-segment outcome in 25")
{
    u32 roll = 0;
    enum Species species = SPECIES_NONE;
    for (u32 i = 0; i < 25; i++)
    {
        PARAMETRIZE { species = SPECIES_DUNSPARCE; roll = i; }
        PARAMETRIZE { species = SPECIES_DUDUNSPARCE; roll = i; }
    }
    EnableForms();
    SET_RNG(RNG_POLISHED_WILD_FORM, roll);
    CreateWildMon(species, 32);
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
    EXPECT_EQ(PolishedGetBoxMonForm(&mon->box), roll == 0 ? 2 : 1);
    EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), species == SPECIES_DUDUNSPARCE && roll == 0 ? SPECIES_DUDUNSPARCE_THREE_SEGMENT : species);
}

TEST("Polished explicit plain encounters and named variants never reroll")
{
    EnableForms();
    SET_RNG(RNG_POLISHED_WILD_FORM, 2);
    CreateWildMonWithForm(SPECIES_EKANS, 5, 1);
    EXPECT_EQ(PolishedGetBoxMonForm(&gParties[B_TRAINER_OPPONENT_A][0].box), 1);
    CreateWildMon(SPECIES_ARBOK_ARIANA, 30);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_ARBOK_ARIANA);
    CreateWildMon(SPECIES_GYARADOS_RED, 30);
    EXPECT_EQ(PolishedGetBoxMonForm(&gParties[B_TRAINER_OPPONENT_A][0].box), 21);
    struct WildPokemon slots[12];
    for (u32 i = 0; i < ARRAY_COUNT(slots); i++)
        slots[i] = (struct WildPokemon) {5, 5, SPECIES_EKANS, 1};
    struct WildPokemonInfo info = {100, slots};
    ZeroPlayerPartyMons();
    EXPECT(TryGenerateWildMon(&info, WILD_AREA_LAND, 0));
    EXPECT_EQ(PolishedGetBoxMonForm(&gParties[B_TRAINER_OPPONENT_A][0].box), 1);
}

TEST("Polished scripted wild creation uses source forms while gifts keep their specified identity")
{
    EnableForms();
    SET_RNG(RNG_POLISHED_WILD_FORM, 20);
    CreateScriptedWildMon(SPECIES_MAGIKARP, 10, ITEM_NONE);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_MAGIKARP_RAINDROP);
    struct Pokemon gift;
    CreateRandomMon(&gift, SPECIES_MAGIKARP, 10);
    EXPECT_EQ(GetMonData(&gift, MON_DATA_SPECIES), SPECIES_MAGIKARP);
    EXPECT_EQ(PolishedGetBoxMonForm(&gift.box), 1);
    CreateRandomMon(&gift, SPECIES_MAGIKARP_SKELLY, 10);
    EXPECT_EQ(PolishedGetBoxMonForm(&gift.box), 2);
    SetConfig(CONFIG_POLISHED_FORMS, FALSE);
    CreateWildMon(SPECIES_MAGIKARP, 10);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_MAGIKARP);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_POLISHED_FORM), 0);
    CreateWildMon(SPECIES_MUDKIP, 5);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_POLISHED_FORM), 0);
}

TEST("Polished NO_FORM evolution preserves every hidden value and checks do not modify saved data")
{
    enum Species species, target;
    u8 form;
    const enum Species arbok[] = {SPECIES_ARBOK, SPECIES_ARBOK_KANTO, SPECIES_ARBOK_KOGA, SPECIES_ARBOK_AGATHA, SPECIES_ARBOK_ARIANA};
    for (u32 i = 1; i <= 5; i++)
        PARAMETRIZE { species = SPECIES_EKANS; target = arbok[i - 1]; form = i; }
    for (u32 i = 1; i <= 21; i++)
        PARAMETRIZE { species = i == 21 ? SPECIES_MAGIKARP : sMagikarpPatterns[i - 1]; target = i == 21 ? SPECIES_GYARADOS_RED : SPECIES_GYARADOS; form = i; }
    PARAMETRIZE { species = SPECIES_DUNSPARCE; target = SPECIES_DUDUNSPARCE; form = 1; }
    PARAMETRIZE { species = SPECIES_DUNSPARCE; target = SPECIES_DUDUNSPARCE_THREE_SEGMENT; form = 2; }
    struct Pokemon mon;
    EnableForms();
    CreateRandomMon(&mon, species, 40);
    SetMonData(&mon, MON_DATA_POLISHED_FORM, &form);
    struct BoxPokemon before = mon.box;
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), target);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, DO_EVO), target);
    EXPECT_EQ(memcmp(&before, &mon.box, sizeof(before)), 0);
    PolishedCommitEvolution(&mon, target);
    SetMonData(&mon, MON_DATA_SPECIES, &target);
    EXPECT_EQ(PolishedGetBoxMonForm(&mon.box), form);
}

static void Parent(struct BoxPokemon *box, enum Species species, u8 gender, u8 form)
{
    CreateBoxMon(box, species, 40, GetMonPersonality(species, gender, NATURE_HARDY, RANDOM_UNOWN_LETTER), OTID_STRUCT_PLAYER_ID);
    SetBoxMonData(box, MON_DATA_POLISHED_FORM, &form);
}

TEST("Polished daycare carries forms from either non-Ditto slot through real egg creation")
{
    enum Species species = SPECIES_NONE, expected = SPECIES_NONE;
    u8 form = 1, slot = 0, gender = MON_MALE;
    for (u32 i = 0; i < 2; i++)
        for (u32 g = 0; g < 2; g++)
        {
            PARAMETRIZE { species = SPECIES_ARBOK_KANTO; expected = SPECIES_EKANS; form = 2; slot = i; gender = g ? MON_FEMALE : MON_MALE; }
            PARAMETRIZE { species = SPECIES_ARBOK_KOGA; expected = SPECIES_EKANS; form = 3; slot = i; gender = g ? MON_FEMALE : MON_MALE; }
            PARAMETRIZE { species = SPECIES_DUDUNSPARCE_THREE_SEGMENT; expected = SPECIES_DUNSPARCE; form = 2; slot = i; gender = g ? MON_FEMALE : MON_MALE; }
            PARAMETRIZE { species = SPECIES_GYARADOS; expected = SPECIES_MAGIKARP_RAINDROP; form = 20; slot = i; gender = g ? MON_FEMALE : MON_MALE; }
            PARAMETRIZE { species = SPECIES_GYARADOS_RED; expected = SPECIES_MAGIKARP; form = 21; slot = i; gender = g ? MON_FEMALE : MON_MALE; }
        }
    EnableForms();
    ZeroPlayerPartyMons();
    struct DayCare *daycare = &gSaveBlock1Ptr->daycare;
    memset(daycare, 0, sizeof(*daycare));
    Parent(&daycare->mons[slot].mon, species, gender, form);
    Parent(&daycare->mons[slot ^ 1].mon, SPECIES_DITTO, MON_GENDERLESS, 0);
    GiveEggFromDaycare();
    struct Pokemon *egg = &gParties[B_TRAINER_PLAYER][0];
    EXPECT_EQ(GetMonData(egg, MON_DATA_IS_EGG), TRUE);
    EXPECT_EQ(GetMonData(egg, MON_DATA_SPECIES), expected);
    EXPECT_EQ(PolishedGetBoxMonForm(&egg->box), form);
    gSpecialVar_0x8004 = 0;
    ScriptHatchMon();
    EXPECT_EQ(GetMonData(egg, MON_DATA_IS_EGG), FALSE);
    EXPECT_EQ(GetMonData(egg, MON_DATA_SPECIES), expected);
    EXPECT_EQ(PolishedGetBoxMonForm(&egg->box), form);
    EXPECT_EQ(GetMonData(egg, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
}

TEST("Polished same-source-species parents share forms equally regardless of variant IDs or Everstone")
{
    u32 parent;
    PARAMETRIZE { parent = 0; }
    PARAMETRIZE { parent = 1; }
    EnableForms();
    SET_RNG(RNG_POLISHED_FORM_PARENT, parent);
    ZeroPlayerPartyMons();
    struct DayCare *daycare = &gSaveBlock1Ptr->daycare;
    memset(daycare, 0, sizeof(*daycare));
    Parent(&daycare->mons[0].mon, SPECIES_MAGIKARP_SKELLY, MON_FEMALE, 2);
    Parent(&daycare->mons[1].mon, SPECIES_MAGIKARP_RAINDROP, MON_MALE, 20);
    enum Item item = ITEM_EVERSTONE;
    SetBoxMonData(&daycare->mons[0].mon, MON_DATA_HELD_ITEM, &item);
    GiveEggFromDaycare();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), parent == 0 ? SPECIES_MAGIKARP_SKELLY : SPECIES_MAGIKARP_RAINDROP);
    EXPECT_EQ(PolishedGetBoxMonForm(&gParties[B_TRAINER_PLAYER][0].box), parent == 0 ? 2 : 20);
}

TEST("Polished different evolution stages inherit the mother's form without a paternal reroll")
{
    EnableForms();
    SET_RNG(RNG_POLISHED_FORM_PARENT, 1);
    ZeroPlayerPartyMons();
    struct DayCare *daycare = &gSaveBlock1Ptr->daycare;
    memset(daycare, 0, sizeof(*daycare));
    Parent(&daycare->mons[0].mon, SPECIES_EKANS, MON_FEMALE, 1);
    Parent(&daycare->mons[1].mon, SPECIES_ARBOK_KANTO, MON_MALE, 2);
    GiveEggFromDaycare();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_EKANS);
    EXPECT_EQ(PolishedGetBoxMonForm(&gParties[B_TRAINER_PLAYER][0].box), 1);
}

TEST("Polished hidden forms occupy only spare encrypted bits and survive all 24 storage orders")
{
    u32 pid = 0;
    for (u32 i = 0; i < 24; i++)
        PARAMETRIZE { pid = i; }
    EnableForms();
    EXPECT_EQ(sizeof(struct PokemonSubstruct0), 12);
    EXPECT_EQ(sizeof(struct BoxPokemon), 80);
    EXPECT_EQ(sizeof(struct Pokemon), 100);
    struct Pokemon mon, restored;
    CreateMonWithIVs(&mon, SPECIES_GYARADOS, 40, pid, OTID_STRUCT_PRESET(0x12345678), 31);
    enum Item item = ITEM_LEFTOVERS;
    SetMonData(&mon, MON_DATA_HELD_ITEM, &item);
    struct BoxPokemon before = mon.box;
    u8 form = 20;
    SetMonData(&mon, MON_DATA_POLISHED_FORM, &form);
    u32 changedWords = 0;
    for (u32 i = 0; i < ARRAY_COUNT(mon.box.secure.raw); i++)
        if (mon.box.secure.raw[i] != before.secure.raw[i])
        {
            EXPECT_EQ(mon.box.secure.raw[i] ^ before.secure.raw[i], (1u ^ 20u) << 26);
            changedWords++;
        }
    EXPECT_EQ(changedWords, 1);
    SetBoxMonAt(0, 0, &mon.box);
    BoxMonToMon(GetBoxedMonPtr(0, 0), &restored);
    EXPECT_EQ(PolishedGetBoxMonForm(&restored.box), 20);
    EXPECT_EQ(GetMonData(&restored, MON_DATA_HELD_ITEM), ITEM_LEFTOVERS);
    EXPECT_EQ(GetMonData(&restored, MON_DATA_HP_IV), 31);
    EXPECT_EQ(GetMonData(&restored, MON_DATA_LEVEL), 40);
    EXPECT_EQ(GetMonData(&restored, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
    EXPECT_EQ(memcmp(&mon.box, &restored.box, sizeof(mon.box)), 0);
}

TEST("Polished old zero-form saves preserve visible identities and never acquire a random hidden form")
{
    EnableForms();
    struct Pokemon mon;
    u8 zero = 0;
    CreateRandomMon(&mon, SPECIES_MAGIKARP_RAINDROP, 20);
    SetMonData(&mon, MON_DATA_POLISHED_FORM, &zero);
    EXPECT_EQ(PolishedGetBoxMonForm(&mon.box), 20);
    PolishedCommitEvolution(&mon, SPECIES_GYARADOS);
    enum Species evolved = SPECIES_GYARADOS;
    SetMonData(&mon, MON_DATA_SPECIES, &evolved);
    EXPECT_EQ(PolishedGetBoxMonForm(&mon.box), 20);
    CreateRandomMon(&mon, SPECIES_DUNSPARCE, 32);
    SetMonData(&mon, MON_DATA_POLISHED_FORM, &zero);
    struct BoxPokemon before = mon.box;
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO), SPECIES_DUDUNSPARCE);
    EXPECT_EQ(memcmp(&before, &mon.box, sizeof(before)), 0);
}

TEST("Polished party and boxed hidden forms survive native flash save and reload")
{
    EnableForms();
    ZeroPlayerPartyMons();
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    CreateRandomMon(mon, SPECIES_GYARADOS, 40);
    u8 form = 20;
    SetMonData(mon, MON_DATA_POLISHED_FORM, &form);
    CalculatePlayerPartyCount();
    SetBoxMonAt(0, 0, &mon->box);
    Save_ResetSaveCounters();
    EXPECT_EQ(TrySavingData(SAVE_NORMAL), SAVE_STATUS_OK);
    ZeroPlayerPartyMons();
    ZeroBoxMonAt(0, 0);
    EXPECT_EQ(LoadGameSave(SAVE_NORMAL), SAVE_STATUS_OK);
    EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), SPECIES_GYARADOS);
    EXPECT_EQ(PolishedGetBoxMonForm(&mon->box), 20);
    EXPECT_EQ(PolishedGetBoxMonForm(GetBoxedMonPtr(0, 0)), 20);
    EXPECT_EQ(GetMonData(mon, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
}
