#include "global.h"
#include "field_move.h"
#include "pokemon.h"
#include "polished_johto_headbutt.h"
#include "constants/species.h"

struct JohtoHeadbuttEntry
{
    u16 species;
    s16 levelValue;
    u8 weight;
    u8 levelMode;
};

struct JohtoHeadbuttDistribution
{
    const struct JohtoHeadbuttEntry *entries;
    u8 count;
};

struct JohtoHeadbuttGroup
{
    struct JohtoHeadbuttDistribution normal;
    struct JohtoHeadbuttDistribution rare;
};

struct JohtoHeadbuttMap
{
    u8 sourceGroup;
    u8 sourceNumber;
    u8 encounterGroup;
};

#include "data/polished_johto_headbutt_tables.h"

enum JohtoHeadbuttEncounterResult JohtoHeadbuttSelectEncounterFromSourceMap(
    u8 sourceMapGroup, u8 sourceMapNumber, u8 facingX, u8 facingY,
    u16 trainerId, u8 badgeCount, u8 battleRoll, u8 slotRoll,
    struct JohtoHeadbuttEncounter *out)
{
    const struct JohtoHeadbuttGroup *group = NULL;
    const struct JohtoHeadbuttDistribution *distribution;
    const struct JohtoHeadbuttEntry *entry;
    enum JohtoHeadbuttTreeScore score;
    u32 i;
    u8 level;

    if (out == NULL || badgeCount > 16 || battleRoll >= 10 || slotRoll >= 100)
        return JOHTO_HEADBUTT_INVALID_INPUT;

    for (i = 0; i < ARRAY_COUNT(sJohtoHeadbuttMaps); i++)
    {
        if (sJohtoHeadbuttMaps[i].sourceGroup == sourceMapGroup
         && sJohtoHeadbuttMaps[i].sourceNumber == sourceMapNumber)
        {
            group = &sJohtoHeadbuttGroups[sJohtoHeadbuttMaps[i].encounterGroup];
            break;
        }
    }
    if (group == NULL)
        return JOHTO_HEADBUTT_NO_ENCOUNTER_SET;

    score = GetJohtoHeadbuttTreeScore(facingX, facingY, trainerId);
    if (!JohtoHeadbuttRollStartsBattle(score, battleRoll))
        return JOHTO_HEADBUTT_NO_ENCOUNTER;
    distribution = JohtoHeadbuttUsesRareEncounterTable(score) ? &group->rare : &group->normal;
    for (i = 0; i < distribution->count; i++)
    {
        entry = &distribution->entries[i];
        if (slotRoll < entry->weight)
        {
            if (!IsSpeciesEnabled(entry->species))
                return JOHTO_HEADBUTT_SPECIES_UNAVAILABLE;
            if (!JohtoHeadbuttResolveLevelFromBadgeCount(entry->levelMode, entry->levelValue, badgeCount, &level))
                return JOHTO_HEADBUTT_INVALID_INPUT;
            out->species = entry->species;
            out->level = level;
            return JOHTO_HEADBUTT_ENCOUNTER;
        }
        slotRoll -= entry->weight;
    }
    return JOHTO_HEADBUTT_INVALID_INPUT;
}
