#include "global.h"
#if POLISHED_JOHTO_DEMO
#include "battle.h"
#include "data.h"
#include "event_data.h"
#include "frontier_util.h"
#include "pokemon.h"
#include "polished_tower.h"
#include "polished_pokemon_data.h"
#include "polished_growth.h"
#include "random.h"
#include "trainer_util.h"
#include "tv.h"
#include "string_util.h"
#include "constants/battle_frontier.h"
#include "constants/frontier_util.h"

#include "constants/battle_frontier_mons.h"
#include "data/polished_tower_sets.h"

// Keep pre-port Factory and Tent indices valid in existing saves.
#define POLISHED_RENTAL_BASE 0x8000
STATIC_ASSERT(NUM_FRONTIER_MONS <= POLISHED_RENTAL_BASE, FactoryIndexOverlap);
STATIC_ASSERT(POLISHED_RENTAL_BASE + ARRAY_COUNT(sTowerSets) <= 0xffff, FactoryIndexOverflow);

bool32 IsPolishedFactoryChallenge(void)
{
    return gSaveBlock2Ptr->frontier.lvlMode == FRONTIER_LVL_50
        && VarGet(VAR_FRONTIER_FACILITY) == FRONTIER_FACILITY_FACTORY
        && VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_SINGLES;
}

const struct PolishedTowerSet *PolishedGetRentalSet(u16 id)
{
    if (id < POLISHED_RENTAL_BASE || id >= POLISHED_RENTAL_BASE + ARRAY_COUNT(sTowerSets))
        return NULL;
    return &sTowerSets[id - POLISHED_RENTAL_BASE];
}

bool32 IsPolishedTowerLevelMode(u32 mode)
{
    return mode == FRONTIER_LVL_50
        && VarGet(VAR_FRONTIER_FACILITY) == FRONTIER_FACILITY_TOWER
        && VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_SINGLES;
}

bool32 IsPolishedTowerChallenge(void)
{
    return IsPolishedTowerLevelMode(gSaveBlock2Ptr->frontier.lvlMode);
}

bool32 IsPolishedTowerSpeciesBanned(enum Species species)
{
    if (species == SPECIES_NONE || species == SPECIES_EGG)
        return TRUE;
    enum Species base = GET_BASE_SPECIES_ID(species);
    switch (base)
    {
    case SPECIES_MEWTWO:
    case SPECIES_MEW:
    case SPECIES_LUGIA:
    case SPECIES_HO_OH:
    case SPECIES_CELEBI:
        return TRUE;
    default:
        // Expansion-only species retain their configured facility rules.
        return !PolishedGetSpeciesBaseData(species) && gSpeciesInfo[species].isFrontierBanned;
    }
}

enum TowerEntryConflict PolishedTowerEntryConflict(enum Species first, enum Item firstItem, enum Species second, enum Item secondItem)
{
    if (GET_BASE_SPECIES_ID(first) == GET_BASE_SPECIES_ID(second))
        return TOWER_ENTRY_DUPLICATE_SPECIES;
    if (firstItem != ITEM_NONE && firstItem == secondItem)
        return TOWER_ENTRY_DUPLICATE_ITEM;
    return TOWER_ENTRY_OK;
}

bool32 PolishedHasEligibleTowerTeam(void)
{
    enum Species species[PARTY_SIZE];
    enum Item items[PARTY_SIZE];
    u32 count = 0;
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        enum Species candidate = GetMonData(mon, MON_DATA_SPECIES_OR_EGG);
        if (IsPolishedTowerSpeciesBanned(candidate))
            continue;
        species[count] = candidate;
        items[count++] = GetMonData(mon, MON_DATA_HELD_ITEM);
    }
    // Up to twenty triples. A greedy choice can hide a valid registration.
    for (u32 i = 0; i < count; i++)
        for (u32 j = i + 1; j < count; j++)
        {
            if (PolishedTowerEntryConflict(species[i], items[i], species[j], items[j]))
                continue;
            for (u32 k = j + 1; k < count; k++)
                if (!PolishedTowerEntryConflict(species[i], items[i], species[k], items[k])
                    && !PolishedTowerEntryConflict(species[j], items[j], species[k], items[k]))
                    return TRUE;
        }
    return FALSE;
}

#define TOWER_HISTORY_VERSION 0x50435431

void PolishedResetTowerHistory(void)
{
    memset(gSaveBlock3Ptr->polishedTowerHistory, 0, sizeof(gSaveBlock3Ptr->polishedTowerHistory));
    gSaveBlock3Ptr->polishedTowerVersion = TOWER_HISTORY_VERSION;
}

static bool32 IsLegalTowerTeam(const u16 *team)
{
    for (u32 slot = 0; slot < FRONTIER_PARTY_SIZE; slot++)
    {
        const struct PolishedTowerSet *set = &sTowerSets[team[slot]];
        for (u32 earlier = 0; earlier < slot; earlier++)
        {
            const struct PolishedTowerSet *other = &sTowerSets[team[earlier]];
            if (set->baseSpecies == other->baseSpecies || (set->item != ITEM_NONE && set->item == other->item))
                return FALSE;
        }
        // CheckSetRepeats decrements b before AND after the inner loop.
        // With three candidates only the first two are compared to history.
        if (slot < FRONTIER_PARTY_SIZE - 1)
            for (u32 i = 0; i < ARRAY_COUNT(gSaveBlock3Ptr->polishedTowerHistory); i++)
                if (gSaveBlock3Ptr->polishedTowerHistory[i] == team[slot] + 1)
                    return FALSE;
    }
    return TRUE;
}

static void CreateTowerMon(struct Pokemon *mon, u32 index, u32 ev, struct TrainerGenerator *generator)
{
    const struct PolishedTowerSet *set = &sTowerSets[index];
    u8 evs[NUM_STATS] = {ev, ev, ev, ev, ev, ((set->iv >> 15) & 31) ? ev : 0};
    struct TrainerMon definition = {
        .species = set->species, .heldItem = set->item, .lvl = 50, .ball = BALL_POKE,
        .gender = set->gender, .nature = set->nature, .friendship = 160,
        .iv = set->iv, .ev = evs, .ability = GetSpeciesAbility(set->species, set->abilitySlot),
    };
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        definition.moves[i] = set->moves[i];
    GenerateMonFromTrainerMon(mon, &definition, generator);
    SetMonData(mon, MON_DATA_ABILITY_NUM, &set->abilitySlot);
}

bool32 PolishedCreateRentalMon(struct Pokemon *mon, u16 id)
{
    if (!PolishedGetRentalSet(id))
        return FALSE;
    struct TrainerGenerator generator = {.name = {EOS}, .otID = OTID_STRUCT_PRESET(READ_OTID_FROM_SAVE),
        .localRngState = LocalRandomSeed(id)};
    CreateTowerMon(mon, id - POLISHED_RENTAL_BASE, 84, &generator);
    return TRUE;
}

static void ShuffleTiers(u8 *tiers, u32 count)
{
    for (u32 last = count - 1; last > 0; last--)
    {
        u32 pick = PolishedRandomRange(last + 1);
        u8 temp;
        SWAP(tiers[last], tiers[pick], temp);
    }
}

static u16 DrawSet(u32 tier)
{
    u32 start = sTowerTierOffsets[tier];
    return start + PolishedRandomRange(sTowerTierOffsets[tier + 1] - start);
}

void PolishedGenerateFactoryRentals(void)
{
    struct BattleFrontier *frontier = &gSaveBlock2Ptr->frontier;
    u32 wins = frontier->factoryWinStreaks[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50] + 5u;
    u32 swaps = frontier->factoryRentsCount[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50];
    u32 row = min(7u, wins / 7 + (wins % 7) / 3 + swaps / 6);
    u8 tiers[PARTY_SIZE];
    for (u32 i = 0; i < PARTY_SIZE; i++)
        tiers[i] = (sTowerTierMasks[row][i / FRONTIER_PARTY_SIZE] >> (2 * (i % FRONTIER_PARTY_SIZE))) & 3;
    ShuffleTiers(tiers, PARTY_SIZE);

    // NewRentalTeam retries the whole draw with the same shuffled tiers.
    // Rental legality compares base species and permits repeated held items.
    u16 team[PARTY_SIZE];
    bool32 legal;
    do
    {
        legal = TRUE;
        for (u32 i = 0; i < PARTY_SIZE; i++)
        {
            u16 set = DrawSet(tiers[i]);
            for (u32 j = 0; j < i; j++)
                if (sTowerSets[set].baseSpecies == sTowerSets[team[j]].baseSpecies)
                    legal = FALSE;
            team[i] = set;
        }
    } while (!legal);
    for (u32 i = 0; i < PARTY_SIZE; i++)
        frontier->rentalMons[i].monId = POLISHED_RENTAL_BASE + team[i];
}

static u32 TowerDifficulty(u32 wins)
{
    static const u8 offsets[] = {0, 1, 1, 2, 2, 3, 4};
    return min(7u, wins / 7 + offsets[wins % 7]);
}

// The release compares the high byte first, then compares low bytes even
// when the old high byte was greater. Its Tower record is not a numeric max.
static u16 UpdateTowerRecord(struct BattleFrontier *frontier, u16 record, u16 wins)
{
    if ((wins >> 8) > (record >> 8))
    {
        // After copying both record bytes, .check_record reads one byte
        // further: the high byte of the current rental streak in source WRAM.
        u16 *streak = &frontier->factoryWinStreaks[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50];
        bool32 active = frontier->winStreakActiveFlags & STREAK_FACTORY_SINGLES_50;
        u16 current = active ? *streak : 0;
        if ((u8)wins > (current >> 8))
        {
            *streak = ((u8)wins << 8) | (u8)current;
            if (!active)
                frontier->factoryRentsCount[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50] = 0;
            frontier->winStreakActiveFlags |= STREAK_FACTORY_SINGLES_50;
        }
        return wins;
    }
    return (record & 0xff00) | max((u8)record, (u8)wins);
}

void BufferPolishedTowerStreaks(void)
{
    const struct BattleFrontier *frontier = &gSaveBlock2Ptr->frontier;
    ConvertIntToDecimalStringN(gStringVar1, frontier->towerWinStreaks[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50], STR_CONV_MODE_LEFT_ALIGN, 5);
    ConvertIntToDecimalStringN(gStringVar2, frontier->towerRecordWinStreaks[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50], STR_CONV_MODE_LEFT_ALIGN, 5);
    StringExpandPlaceholders(gStringVar4, COMPOUND_STRING("Streak: {STR_VAR_1} wins\nRecord: {STR_VAR_2} wins"));
}

void PolishedSettleTowerPoints(void)
{
    if (!IsPolishedTowerChallenge())
        return;
    struct BattleFrontier *frontier = &gSaveBlock2Ptr->frontier;
    u32 points = 0;
    if (frontier->challengeStatus == CHALLENGE_STATUS_WON || frontier->challengeStatus == CHALLENGE_STATUS_LOST
        || frontier->challengeStatus == CHALLENGE_STATUS_SAVING)
    {
        u32 stages = min(frontier->curChallengeBattleNum, FRONTIER_STAGES_PER_CHALLENGE);
        u16 wins = frontier->towerWinStreaks[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50];
        // Each completed battle already advanced the native streak. The source
        // commits all of their BP in the lobby, including an incomplete set.
        u16 openingStreak = wins - stages;
        for (u32 i = 0; i < stages; i++)
            points += 1 + TowerDifficulty(openingStreak + i);
        frontier->battlePoints = min(MAX_BATTLE_FRONTIER_POINTS, frontier->battlePoints + points);
        frontier->cardBattlePoints = min(0xffffu, frontier->cardBattlePoints + points);
        if (points)
            IncrementDailyBattlePoints(points);
        u16 *record = &frontier->towerRecordWinStreaks[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50];
        *record = UpdateTowerRecord(frontier, *record, wins);
        if (frontier->challengeStatus != CHALLENGE_STATUS_WON)
            frontier->towerWinStreaks[FRONTIER_MODE_SINGLES][FRONTIER_LVL_50] = 0;
        frontier->challengeStatus = 0;
    }
    ConvertIntToDecimalStringN(gStringVar1, points, STR_CONV_MODE_LEFT_ALIGN, 2);
}

void PolishedForfeitInterruptedTowerSet(void)
{
    if (!IsPolishedTowerChallenge() || gSaveBlock2Ptr->frontier.challengeStatus != CHALLENGE_STATUS_SAVING)
        return;
    PolishedSettleTowerPoints();
    gSaveBlock2Ptr->frontier.winStreakActiveFlags &= ~STREAK_TOWER_SINGLES_50;
    SaveGameFrontier();
}

void PolishedBuildTowerParty(u32 wins)
{
    // A set that starts near 65535 retains its difficulty until settlement,
    // even when the GBA's running 16-bit streak wraps during that set.
    u32 stages = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
    wins = (u16)(wins - stages) + stages;
    // Each battle starts from the lobby registration, including held items
    // and move slots. Keep the original party in the regular save block.
    for (u32 i = 0; i < min(gPartiesCount[B_TRAINER_PLAYER], FRONTIER_PARTY_SIZE); i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u32 slot = gSaveBlock2Ptr->frontier.selectedPartyMons[i];
        if (slot && slot <= *GetSavedPlayerPartyCount())
            *mon = *GetSavedPlayerPartyMon(slot - 1);
        enum Species species = GetMonData(mon, MON_DATA_SPECIES);
        u32 experience = GetSpeciesExperienceAtLevel(species, FRONTIER_MAX_LEVEL_50);
        SetMonData(mon, MON_DATA_EXP, &experience);
        CalculateMonStats(mon);
        HealPokemon(mon);
    }
    u32 difficulty = TowerDifficulty(wins);
    u32 mask = sTowerTierMasks[difficulty][Random() & 1];
    u8 tiers[FRONTIER_PARTY_SIZE];
    u16 team[FRONTIER_PARTY_SIZE];
    u16 *history = gSaveBlock3Ptr->polishedTowerHistory;
    if (gSaveBlock3Ptr->polishedTowerVersion != TOWER_HISTORY_VERSION)
        PolishedResetTowerHistory();
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++, mask >>= 2)
        tiers[i] = mask & 3;
    ShuffleTiers(tiers, FRONTIER_PARTY_SIZE);
    do
    {
        for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
            team[i] = DrawSet(tiers[i]);
    } while (!IsLegalTowerTeam(team));

    memmove(history, history + FRONTIER_PARTY_SIZE, sizeof(gSaveBlock3Ptr->polishedTowerHistory) - sizeof(team));
    struct TrainerGenerator generator = {.name = {EOS}, .otID = OTID_STRUCT_PRESET(0),
        .localRngState = LocalRandomSeed(team[0] + 1)};
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        history[ARRAY_COUNT(gSaveBlock3Ptr->polishedTowerHistory) - FRONTIER_PARTY_SIZE + i] = team[i] + 1;
        CreateTowerMon(&gParties[B_TRAINER_OPPONENT_A][i], team[i], min(wins * 16, 252u) / 3, &generator);
    }
}
#endif
