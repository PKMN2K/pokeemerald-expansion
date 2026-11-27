#ifndef GUARD_POLISHED_TOWER_H
#define GUARD_POLISHED_TOWER_H

#if POLISHED_JOHTO_DEMO
bool32 IsPolishedTowerLevelMode(u32 mode);
bool32 IsPolishedTowerChallenge(void);
bool32 IsPolishedTowerSpeciesBanned(enum Species species);
enum TowerEntryConflict { TOWER_ENTRY_OK, TOWER_ENTRY_DUPLICATE_SPECIES, TOWER_ENTRY_DUPLICATE_ITEM };
enum TowerEntryConflict PolishedTowerEntryConflict(enum Species first, enum Item firstItem, enum Species second, enum Item secondItem);
bool32 PolishedHasEligibleTowerTeam(void);
void PolishedSettleTowerPoints(void);
void PolishedResetTowerHistory(void);
void PolishedBuildTowerParty(u32 wins);
#endif

#endif
