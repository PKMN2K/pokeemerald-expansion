#include "global.h"
#include "constants/maps.h"
#include "battle.h"
#include "wild_encounter.h"
#include "polished_escape.h"
#include "polished_pokemon_data.h"
#include "event_data.h"
#include "ow_abilities.h"
#include "pokemon.h"
#include "random.h"
#include "roamer.h"

// Despite having a variable to track it, the roamer is
// hard-coded to only ever be in map group 0
#define ROAMER_MAP_GROUP 0

enum
{
    MAP_GRP, // map group
    MAP_NUM, // map number
};

struct Roamer *GetRoamerByIndex(u32 index)
{
#if POLISHED_JOHTO_DEMO && ROAMER_COUNT < 2
    if (index >= ROAMER_COUNT)
    {
        if (gSaveBlock3Ptr->polishedRoamerMagic != 0x50524231) // PRB1
        {
            memset(gSaveBlock3Ptr->extraRoamers, 0, sizeof(gSaveBlock3Ptr->extraRoamers));
            gSaveBlock3Ptr->polishedRoamerMagic = 0x50524231;
        }
        return &gSaveBlock3Ptr->extraRoamers[index - ROAMER_COUNT];
    }
#endif
    return &gSaveBlock1Ptr->roamer[index];
}

#define ROAMER(index) GetRoamerByIndex(index)
EWRAM_DATA static u8 sLocationHistory[ACTIVE_ROAMER_SLOTS][3][2] = {0};
EWRAM_DATA static u8 sRoamerLocation[ACTIVE_ROAMER_SLOTS][2] = {0};
EWRAM_DATA u8 gEncounteredRoamerIndex = 0;

#define ___ MAP_NUM(MAP_UNDEFINED) // For empty spots in the location table

// Note: There are two potential softlocks that can occur with this table if its maps are
//       changed in particular ways. They can be avoided by ensuring the following:
//       - There must be at least 2 location sets that start with a different map,
//         i.e. every location set cannot start with the same map. This is because of
//         the while loop in RoamerMoveToOtherLocationSet.
//       - Each location set must have at least 3 unique maps. This is because of
//         the while loop in RoamerMove. In this loop the first map in the set is
//         ignored, and an additional map is ignored if the roamer was there recently.
//       - Additionally, while not a softlock, it's worth noting that if for any
//         map in the location table there is not a location set that starts with
//         that map then the roamer will be significantly less likely to move away
//         from that map when it lands there.
static const u8 sRoamerLocations[][6] =
{
    { MAP_NUM(MAP_ROUTE110), MAP_NUM(MAP_ROUTE111), MAP_NUM(MAP_ROUTE117), MAP_NUM(MAP_ROUTE118), MAP_NUM(MAP_ROUTE134), ___ },
    { MAP_NUM(MAP_ROUTE111), MAP_NUM(MAP_ROUTE110), MAP_NUM(MAP_ROUTE117), MAP_NUM(MAP_ROUTE118), ___, ___ },
    { MAP_NUM(MAP_ROUTE117), MAP_NUM(MAP_ROUTE111), MAP_NUM(MAP_ROUTE110), MAP_NUM(MAP_ROUTE118), ___, ___ },
    { MAP_NUM(MAP_ROUTE118), MAP_NUM(MAP_ROUTE117), MAP_NUM(MAP_ROUTE110), MAP_NUM(MAP_ROUTE111), MAP_NUM(MAP_ROUTE119), MAP_NUM(MAP_ROUTE123) },
    { MAP_NUM(MAP_ROUTE119), MAP_NUM(MAP_ROUTE118), MAP_NUM(MAP_ROUTE120), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE120), MAP_NUM(MAP_ROUTE119), MAP_NUM(MAP_ROUTE121), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE121), MAP_NUM(MAP_ROUTE120), MAP_NUM(MAP_ROUTE122), MAP_NUM(MAP_ROUTE123), ___, ___ },
    { MAP_NUM(MAP_ROUTE122), MAP_NUM(MAP_ROUTE121), MAP_NUM(MAP_ROUTE123), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE123), MAP_NUM(MAP_ROUTE122), MAP_NUM(MAP_ROUTE118), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE124), MAP_NUM(MAP_ROUTE121), MAP_NUM(MAP_ROUTE125), MAP_NUM(MAP_ROUTE126), ___, ___ },
    { MAP_NUM(MAP_ROUTE125), MAP_NUM(MAP_ROUTE124), MAP_NUM(MAP_ROUTE127), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE126), MAP_NUM(MAP_ROUTE124), MAP_NUM(MAP_ROUTE127), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE127), MAP_NUM(MAP_ROUTE125), MAP_NUM(MAP_ROUTE126), MAP_NUM(MAP_ROUTE128), ___, ___ },
    { MAP_NUM(MAP_ROUTE128), MAP_NUM(MAP_ROUTE127), MAP_NUM(MAP_ROUTE129), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE129), MAP_NUM(MAP_ROUTE128), MAP_NUM(MAP_ROUTE130), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE130), MAP_NUM(MAP_ROUTE129), MAP_NUM(MAP_ROUTE131), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE131), MAP_NUM(MAP_ROUTE130), MAP_NUM(MAP_ROUTE132), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE132), MAP_NUM(MAP_ROUTE131), MAP_NUM(MAP_ROUTE133), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE133), MAP_NUM(MAP_ROUTE132), MAP_NUM(MAP_ROUTE134), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE134), MAP_NUM(MAP_ROUTE133), MAP_NUM(MAP_ROUTE110), ___, ___, ___ },
    { ___, ___, ___, ___, ___, ___ },
};

#undef ___
#define NUM_LOCATION_SETS (ARRAY_COUNT(sRoamerLocations) - 1)
#define NUM_LOCATIONS_PER_SET (ARRAY_COUNT(sRoamerLocations[0]))

struct PolishedRoamMap
{
    u8 route;
    u8 count;
    u8 destinations[4];
};

#include "data/polished_roam_maps.h"

static u32 StartingBeastRoute(const struct Roamer *beast)
{
    if (!P_POLISHED_SPECIES_DATA)
        return 0;
    if (beast->species == SPECIES_RAIKOU)
        return 42;
    if (beast->species == SPECIES_ENTEI)
        return 37;
    return 0;
}

// The complete graph uses source route IDs even before a route has a GBA map.
static u16 BeastRouteMap(u32 route)
{
    switch (route)
    {
    case 29: return MAP_POLISHED_ROUTE_29;
    case 30: return MAP_POLISHED_ROUTE_30;
    case 31: return MAP_POLISHED_ROUTE_31;
    default: return MAP_UNDEFINED;
    }
}

static u32 BeastRoute(const struct Roamer *beast)
{
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedRoamMaps); i++)
        if (sPolishedRoamMaps[i].route == beast->polishedRoamRoute)
            return beast->polishedRoamRoute;
    return StartingBeastRoute(beast); // A legacy save has no source route byte.
}

static void JumpBeast(struct Roamer *beast)
{
    u16 playerMap = (u8)gSaveBlock1Ptr->location.mapGroup << 8 | (u8)gSaveBlock1Ptr->location.mapNum;
    u32 route;
    do
        route = sPolishedRoamMaps[Random() & 15].route;
    while (playerMap != MAP_UNDEFINED && BeastRouteMap(route) == playerMap);
    beast->polishedRoamRoute = route;
}

static void MoveBeast(struct Roamer *beast)
{
    u32 route = BeastRoute(beast);
    for (u32 i = 0; i < ARRAY_COUNT(sPolishedRoamMaps); i++)
    {
        const struct PolishedRoamMap *map = &sPolishedRoamMaps[i];
        if (map->route != route)
            continue;
        u32 roll;
        do
        {
            roll = Random() & 31;
            if (!roll)
            {
                JumpBeast(beast);
                return;
            }
            roll &= 3;
        } while (roll >= map->count);
        beast->polishedRoamRoute = map->destinations[roll];
        return;
    }
}

void DeactivateAllRoamers(void)
{
    u32 i;

    for (i = 0; i < ACTIVE_ROAMER_SLOTS; i++)
        SetRoamerInactive(i);
}

static void ClearRoamerLocationHistory(u32 roamerIndex)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sLocationHistory[roamerIndex]); i++)
    {
        sLocationHistory[roamerIndex][i][MAP_GRP] = 0;
        sLocationHistory[roamerIndex][i][MAP_NUM] = 0;
    }
}

void MoveAllRoamersToOtherLocationSets(void)
{
    u32 i;

    for (i = 0; i < ACTIVE_ROAMER_SLOTS; i++)
        RoamerMoveToOtherLocationSet(i);
}

void MoveAllRoamers(void)
{
    u32 i;

    for (i = 0; i < ACTIVE_ROAMER_SLOTS; i++)
        RoamerMove(i);
}

static void CreateInitialRoamerMon(u8 index, enum Species species, u8 level)
{
    ClearRoamerLocationHistory(index);
    if (!TryCreatePolishedWildMon(&gParties[B_TRAINER_OPPONENT_A][0], species, level))
    {
        u32 personality = GetMonPersonality(species,
            GetSynchronizedGender(ROAMER_ORIGIN, species),
            GetSynchronizedNature(ROAMER_ORIGIN, species),
            RANDOM_UNOWN_LETTER);
        CreateMonWithIVs(&gParties[B_TRAINER_OPPONENT_A][0], species, level, personality, OTID_STRUCT_PLAYER_ID, USE_RANDOM_IVS);
    }
    GiveMonInitialMoveset(&gParties[B_TRAINER_OPPONENT_A][0]);
    ROAMER(index)->polishedAbilitySlotPlusOne = P_POLISHED_SPECIES_DATA && PolishedGetSpeciesBaseData(species) != NULL
        ? 1 + GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_ABILITY_NUM) : 0;
    ROAMER(index)->ivs = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_IVS);
    ROAMER(index)->personality = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_PERSONALITY);
    ROAMER(index)->species = species;
    ROAMER(index)->level = level;
    ROAMER(index)->statusA = 0;
    ROAMER(index)->statusB = 0;
    ROAMER(index)->hp = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_MAX_HP);
    ROAMER(index)->cool = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_COOL);
    ROAMER(index)->beauty = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_BEAUTY);
    ROAMER(index)->cute = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_CUTE);
    ROAMER(index)->smart = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SMART);
    ROAMER(index)->tough = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_TOUGH);
    ROAMER(index)->shiny = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_IS_SHINY);
    ROAMER(index)->active = TRUE;
    ROAMER(index)->polishedRoamRoute = StartingBeastRoute(ROAMER(index));
    if (ROAMER(index)->polishedRoamRoute)
        return;
    sRoamerLocation[index][MAP_GRP] = ROAMER_MAP_GROUP;
    sRoamerLocation[index][MAP_NUM] = sRoamerLocations[Random() % NUM_LOCATION_SETS][0];
}

static u8 GetFirstInactiveRoamerIndex(void)
{
    u32 i;

    for (i = 0; i < ACTIVE_ROAMER_SLOTS; i++)
    {
        if (!ROAMER(i)->active)
            return i;
    }
    return ACTIVE_ROAMER_SLOTS;
}

bool8 TryAddRoamer(enum Species species, u8 level)
{
    u8 index = GetFirstInactiveRoamerIndex();

    if (index < ACTIVE_ROAMER_SLOTS)
    {
        // Create the roamer and stop searching
        CreateInitialRoamerMon(index, species, level);
        return TRUE;
    }

    // Maximum active roamers found: do nothing and let the calling function know
    return FALSE;
}

// gSpecialVar_0x8004 here corresponds to the options in the multichoice MULTI_TV_LATI (0 for 'Red', 1 for 'Blue')
void InitRoamer(void)
{
    if (gSpecialVar_0x8004 == 0) // Red
        TryAddRoamer(SPECIES_LATIAS, 40);
    else
        TryAddRoamer(SPECIES_LATIOS, 40);
}

void UpdateLocationHistoryForRoamer(void)
{
    u32 i;

    for (i = 0; i < ACTIVE_ROAMER_SLOTS; i++)
    {
        sLocationHistory[i][2][MAP_GRP] = sLocationHistory[i][1][MAP_GRP];
        sLocationHistory[i][2][MAP_NUM] = sLocationHistory[i][1][MAP_NUM];

        sLocationHistory[i][1][MAP_GRP] = sLocationHistory[i][0][MAP_GRP];
        sLocationHistory[i][1][MAP_NUM] = sLocationHistory[i][0][MAP_NUM];

        sLocationHistory[i][0][MAP_GRP] = gSaveBlock1Ptr->location.mapGroup;
        sLocationHistory[i][0][MAP_NUM] = gSaveBlock1Ptr->location.mapNum;
    }
}

void RoamerMoveToOtherLocationSet(u32 roamerIndex)
{
    u8 mapNum = 0;

    if (!ROAMER(roamerIndex)->active)
        return;

    if (StartingBeastRoute(ROAMER(roamerIndex)))
    {
        JumpBeast(ROAMER(roamerIndex));
        return;
    }

    sRoamerLocation[roamerIndex][MAP_GRP] = ROAMER_MAP_GROUP;

    // Choose a location set that starts with a map
    // different from the roamer's current map
    do
    {
        mapNum = sRoamerLocations[Random() % NUM_LOCATION_SETS][0];
        if (sRoamerLocation[roamerIndex][MAP_NUM] != mapNum)
        {
            sRoamerLocation[roamerIndex][MAP_NUM] = mapNum;
            return;
        }
    } while (sRoamerLocation[roamerIndex][MAP_NUM] == mapNum);
    sRoamerLocation[roamerIndex][MAP_NUM] = mapNum;
}

void RoamerMove(u32 roamerIndex)
{
    if (ROAMER(roamerIndex)->active && StartingBeastRoute(ROAMER(roamerIndex)))
    {
        MoveBeast(ROAMER(roamerIndex));
        return;
    }

#if POLISHED_JOHTO_DEMO && ROAMER_COUNT < 2
    if (roamerIndex >= ROAMER_COUNT && !ROAMER(roamerIndex)->active)
        return;
#endif
    u8 locSet = 0;

    if ((Random() % 16) == 0)
    {
        RoamerMoveToOtherLocationSet(roamerIndex);
    }
    else
    {
        if (!ROAMER(roamerIndex)->active)
            return;

        while (locSet < NUM_LOCATION_SETS)
        {
            // Find the location set that starts with the roamer's current map
            if (sRoamerLocation[roamerIndex][MAP_NUM] == sRoamerLocations[locSet][0])
            {
                u8 mapNum;
                // Choose a new map (excluding the first) within this set
                // Also exclude a map if the roamer was there 2 moves ago
                do
                {
                    mapNum = sRoamerLocations[locSet][(Random() % (NUM_LOCATIONS_PER_SET - 1)) + 1];
                } while ((sLocationHistory[roamerIndex][2][MAP_GRP] == ROAMER_MAP_GROUP
                        && sLocationHistory[roamerIndex][2][MAP_NUM] == mapNum)
                        || mapNum == MAP_NUM(MAP_UNDEFINED));
                sRoamerLocation[roamerIndex][MAP_NUM] = mapNum;
                return;
            }
            locSet++;
        }
    }
}

bool8 IsRoamerAt(u32 roamerIndex, u8 mapGroup, u8 mapNum)
{
    if (ROAMER(roamerIndex)->active && StartingBeastRoute(ROAMER(roamerIndex)))
    {
        u16 map = BeastRouteMap(BeastRoute(ROAMER(roamerIndex)));
        return map != MAP_UNDEFINED && MAP_GROUP(map) == mapGroup && MAP_NUM(map) == mapNum;
    }
    if (ROAMER(roamerIndex)->active && mapGroup == sRoamerLocation[roamerIndex][MAP_GRP] && mapNum == sRoamerLocation[roamerIndex][MAP_NUM])
        return TRUE;
    else
        return FALSE;
}

void CreateRoamerMonInstance(u32 roamerIndex)
{
    u32 status = ROAMER(roamerIndex)->statusA + (ROAMER(roamerIndex)->statusB << 8);
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
    ZeroEnemyPartyMons();
    CreateMonWithIVsPersonality(mon, ROAMER(roamerIndex)->species, ROAMER(roamerIndex)->level, ROAMER(roamerIndex)->ivs, ROAMER(roamerIndex)->personality);
    u32 slot = ROAMER(roamerIndex)->polishedAbilitySlotPlusOne;
    if (P_POLISHED_SPECIES_DATA && slot >= 1 && slot <= 3 && PolishedGetSpeciesBaseData(ROAMER(roamerIndex)->species) != NULL)
    {
        slot--;
        SetMonData(mon, MON_DATA_ABILITY_NUM, &slot);
    }
    SetMonData(mon, MON_DATA_STATUS, &status);
    SetMonData(mon, MON_DATA_HP, &ROAMER(roamerIndex)->hp);
    SetMonData(mon, MON_DATA_COOL, &ROAMER(roamerIndex)->cool);
    SetMonData(mon, MON_DATA_BEAUTY, &ROAMER(roamerIndex)->beauty);
    SetMonData(mon, MON_DATA_CUTE, &ROAMER(roamerIndex)->cute);
    SetMonData(mon, MON_DATA_SMART, &ROAMER(roamerIndex)->smart);
    SetMonData(mon, MON_DATA_TOUGH, &ROAMER(roamerIndex)->tough);
    SetMonData(mon, MON_DATA_IS_SHINY, &ROAMER(roamerIndex)->shiny);
}

bool8 TryStartRoamerEncounterInArea(bool32 ordinaryLand)
{
    u32 i;
    u32 selection = 0;
    bool32 rolled = FALSE;

    for (i = 0; i < ACTIVE_ROAMER_SLOTS; i++)
    {
        if (!ROAMER(i)->active)
            continue;
        if (StartingBeastRoute(ROAMER(i)))
        {
            if (!ordinaryLand)
                continue;
            if (!rolled)
            {
                u8 byte = Random();
                selection = byte < 100 ? byte & 3 : 0;
                rolled = TRUE;
            }
            if (selection != (ROAMER(i)->species == SPECIES_RAIKOU ? 1u : 2u))
                continue;
            if (!IsRoamerAt(i, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum))
                continue;
        }
        else if (!IsRoamerAt(i, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum) || Random() % 4 != 0)
            continue;
        CreateRoamerMonInstance(i);
        gEncounteredRoamerIndex = i;
        return TRUE;
    }
    return FALSE;
}

bool8 TryStartRoamerEncounter(void)
{
    return TryStartRoamerEncounterInArea(TRUE);
}

// ExitBattle also gives both beasts a 1/16 movement roll after other battles.
void MovePolishedRoamingBeasts(bool32 roamingBattle)
{
    bool32 rolled = FALSE;
    for (u32 i = 0; i < ACTIVE_ROAMER_SLOTS; i++)
    {
        struct Roamer *beast = ROAMER(i);
        if (!beast->active || !StartingBeastRoute(beast))
            continue;
        if (!roamingBattle && !rolled && (Random() & 15))
            return;
        rolled = TRUE;
        MoveBeast(beast);
    }
}

void UpdateRoamerHPStatus(struct Pokemon *mon)
{
    u32 status = GetMonData(mon, MON_DATA_STATUS);
    if (B_POLISHED_MOVE_BALANCE && PolishedGetSpeciesBaseData(GetMonData(mon, MON_DATA_SPECIES)) != NULL)
        status = PolishedRoamerSavedStatus(status);

    ROAMER(gEncounteredRoamerIndex)->hp = GetMonData(mon, MON_DATA_HP);
    ROAMER(gEncounteredRoamerIndex)->statusA = status;
    ROAMER(gEncounteredRoamerIndex)->statusB = status >> 8;

    if (StartingBeastRoute(ROAMER(gEncounteredRoamerIndex)))
    {
        if (gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_CAUGHT)
        {
            ROAMER(gEncounteredRoamerIndex)->hp = 0;
            SetRoamerInactive(gEncounteredRoamerIndex);
        }
        else
            MovePolishedRoamingBeasts(TRUE);
    }
    else
        RoamerMoveToOtherLocationSet(gEncounteredRoamerIndex);
}

void SetRoamerInactive(u32 roamerIndex)
{
    ROAMER(roamerIndex)->active = FALSE;
}

void GetRoamerLocation(u32 roamerIndex, u8 *mapGroup, u8 *mapNum)
{
    if (ROAMER(roamerIndex)->active && StartingBeastRoute(ROAMER(roamerIndex)))
    {
        u16 map = BeastRouteMap(BeastRoute(ROAMER(roamerIndex)));
        *mapGroup = MAP_GROUP(map);
        *mapNum = MAP_NUM(map);
        return;
    }
    *mapGroup = sRoamerLocation[roamerIndex][MAP_GRP];
    *mapNum = sRoamerLocation[roamerIndex][MAP_NUM];
}
