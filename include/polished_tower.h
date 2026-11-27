#ifndef GUARD_POLISHED_TOWER_H
#define GUARD_POLISHED_TOWER_H

#if POLISHED_JOHTO_DEMO
struct PolishedTowerSet
{
    u16 species, baseSpecies, item, moves[MAX_MON_MOVES];
    u32 iv;
    u8 gender, nature, abilitySlot;
};

bool32 IsPolishedFactoryChallenge(void);
void PolishedGenerateFactoryRentals(void);
void PolishedGenerateFactoryOpponent(void);
void BufferPolishedFactoryScout(void);
const struct PolishedTowerSet *PolishedGetRentalSet(u16 id);
bool32 PolishedCreateRentalMon(struct Pokemon *mon, u16 id);
bool32 IsPolishedTowerLevelMode(u32 mode);
bool32 IsPolishedTowerChallenge(void);
bool32 IsPolishedTowerSpeciesBanned(enum Species species);
enum TowerEntryConflict { TOWER_ENTRY_OK, TOWER_ENTRY_DUPLICATE_SPECIES, TOWER_ENTRY_DUPLICATE_ITEM };
enum TowerEntryConflict PolishedTowerEntryConflict(enum Species first, enum Item firstItem, enum Species second, enum Item secondItem);
bool32 PolishedHasEligibleTowerTeam(void);
void BufferPolishedTowerStreaks(void);
void PolishedSettleTowerPoints(void);
void PolishedSettleFactoryPoints(void);
void PolishedForfeitInterruptedFactorySet(void);
u32 PolishedFactoryDifficulty(u32 wins);
void PolishedForfeitInterruptedTowerSet(void);
void PolishedResetTowerHistory(void);
void PolishedBuildTowerParty(u32 wins);
#endif

#endif
