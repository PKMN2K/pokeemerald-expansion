#include "global.h"
#include "pokemon.h"
#include "polished_johto_headbutt.h"
#include "random.h"
#include "test/test.h"

TEST("Johto Headbutt preserves Hisuian identity when creating an Expansion Pokemon")
{
    struct JohtoHeadbuttEncounter out = {0};
    struct Pokemon mon;
    enum Species species = SPECIES_VOLTORB_HISUI;
    u8 group = 4, number = 14, slot = 90, level = 10;
    enum JohtoHeadbuttEncounterResult result;

    // Pinned Route 37 and Noisy Forest source IDs; not Emerald map IDs.
    PARAMETRIZE { species = SPECIES_VOLTORB_HISUI; group = 4; number = 14; slot = 90; level = 10; }
    PARAMETRIZE { species = SPECIES_ELECTRODE_HISUI; group = 31; number = 7; slot = 65; level = 4; }

    result = JohtoHeadbuttSelectEncounterFromSourceMap(group, number, 0, 0, 6, 0, 0, slot, &out);
    if (!IsSpeciesEnabled(species))
    {
        EXPECT_EQ(result, JOHTO_HEADBUTT_SPECIES_UNAVAILABLE);
    }
    else
    {
        EXPECT_EQ(result, JOHTO_HEADBUTT_ENCOUNTER);
        EXPECT_EQ(out.species, species);
        EXPECT_EQ(out.level, level);
        EXPECT(gSpeciesInfo[out.species].isHisuianForm);
        EXPECT_EQ(GetSpeciesType(out.species, 0), TYPE_ELECTRIC);
        EXPECT_EQ(GetSpeciesType(out.species, 1), TYPE_GRASS);
        CreateMon(&mon, out.species, out.level, 0, OTID_STRUCT_PRESET(0));
        EXPECT_EQ(GetMonData(&mon, MON_DATA_SPECIES), species);
        EXPECT_EQ(GetMonData(&mon, MON_DATA_LEVEL), level);
    }
}

TEST("Johto Headbutt resolves source badge levels without consuming game RNG")
{
    struct JohtoHeadbuttEncounter out = {0};
    rng_value_t rng = gRngValue;
    rng_value_t rng2 = gRng2Value;
    u8 badges = 0, level = 2;
    enum JohtoHeadbuttEncounterResult result;

    PARAMETRIZE { badges = 0; level = 2; }
    PARAMETRIZE { badges = 8; level = 33; }
    PARAMETRIZE { badges = 16; level = 58; }
    result = JohtoHeadbuttSelectEncounterFromSourceMap(31, 7, 0, 0, 6, badges, 0, 0, &out);
    EXPECT_EQ(memcmp(&gRngValue, &rng, sizeof(rng)), 0);
    EXPECT_EQ(memcmp(&gRng2Value, &rng2, sizeof(rng2)), 0);
    if (IsSpeciesEnabled(SPECIES_PINECO))
    {
        EXPECT_EQ(result, JOHTO_HEADBUTT_ENCOUNTER);
        EXPECT_EQ(out.species, SPECIES_PINECO);
        EXPECT_EQ(out.level, level);
    }
    else
    {
        EXPECT_EQ(result, JOHTO_HEADBUTT_SPECIES_UNAVAILABLE);
    }
}

TEST("Johto Headbutt distinguishes absent maps, failed rolls and invalid input")
{
    struct JohtoHeadbuttEncounter out = {SPECIES_PIKACHU, 42};
    EXPECT_EQ(JohtoHeadbuttSelectEncounterFromSourceMap(0, 0, 0, 0, 0, 0, 0, 0, &out), JOHTO_HEADBUTT_NO_ENCOUNTER_SET);
    EXPECT_EQ(JohtoHeadbuttSelectEncounterFromSourceMap(4, 14, 0, 0, 0, 0, 8, 0, &out), JOHTO_HEADBUTT_NO_ENCOUNTER);
    EXPECT_EQ(JohtoHeadbuttSelectEncounterFromSourceMap(4, 14, 0, 0, 0, 17, 0, 0, &out), JOHTO_HEADBUTT_INVALID_INPUT);
    EXPECT_EQ(JohtoHeadbuttSelectEncounterFromSourceMap(4, 14, 0, 0, 0, 0, 0, 100, &out), JOHTO_HEADBUTT_INVALID_INPUT);
    EXPECT_EQ(out.species, SPECIES_PIKACHU);
    EXPECT_EQ(out.level, 42);
}
