#include "global.h"
#if POLISHED_JOHTO_DEMO
#include "battle.h"
#include "data.h"
#include "event_data.h"
#include "pokemon.h"
#include "polished_tower.h"
#include "random.h"
#include "trainer_util.h"
#include "constants/battle_frontier.h"

struct PolishedTowerSet
{
    u16 species, baseSpecies, item, moves[MAX_MON_MOVES];
    u32 iv;
    u8 gender, nature, abilitySlot;
};

#include "data/polished_tower_sets.h"

bool32 IsPolishedTowerChallenge(void)
{
    return gSaveBlock2Ptr->frontier.lvlMode == FRONTIER_LVL_50
        && VarGet(VAR_FRONTIER_FACILITY) == FRONTIER_FACILITY_TOWER
        && VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_SINGLES;
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

static void CreateTowerMon(struct Pokemon *mon, u32 index, u32 wins, struct TrainerGenerator *generator)
{
    const struct PolishedTowerSet *set = &sTowerSets[index];
    u32 ev = min(wins, 16u) * 16;
    ev = min(ev, 252u) / 3; // DefaultOptions selects modern EVs.
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

void PolishedBuildTowerParty(u32 wins)
{
    static const u8 pointOffsets[] = {0, 1, 1, 2, 2, 3, 4};
    u32 difficulty = min(7u, wins / 7 + pointOffsets[wins % 7]);
    u32 mask = sTowerTierMasks[difficulty][Random() & 1];
    u8 tiers[FRONTIER_PARTY_SIZE];
    u16 team[FRONTIER_PARTY_SIZE];
    u16 *history = gSaveBlock3Ptr->polishedTowerHistory;
    if (gSaveBlock3Ptr->polishedTowerVersion != TOWER_HISTORY_VERSION)
        PolishedResetTowerHistory();
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++, mask >>= 2)
        tiers[i] = mask & 3;
    for (u32 last = FRONTIER_PARTY_SIZE - 1; last > 0; last--)
    {
        u32 pick = PolishedRandomRange(last + 1);
        u8 temp;
        SWAP(tiers[last], tiers[pick], temp);
    }
    do
    {
        for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
        {
            u32 start = sTowerTierOffsets[tiers[i]];
            team[i] = start + PolishedRandomRange(sTowerTierOffsets[tiers[i] + 1] - start);
        }
    } while (!IsLegalTowerTeam(team));

    memmove(history, history + FRONTIER_PARTY_SIZE, sizeof(gSaveBlock3Ptr->polishedTowerHistory) - sizeof(team));
    struct TrainerGenerator generator = {.name = {EOS}, .otID = OTID_STRUCT_PRESET(0),
        .localRngState = LocalRandomSeed(team[0] + 1)};
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        history[ARRAY_COUNT(gSaveBlock3Ptr->polishedTowerHistory) - FRONTIER_PARTY_SIZE + i] = team[i] + 1;
        CreateTowerMon(&gParties[B_TRAINER_OPPONENT_A][i], team[i], wins, &generator);
    }
}
#endif
