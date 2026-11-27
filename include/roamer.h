#ifndef GUARD_ROAMER_H
#define GUARD_ROAMER_H

// Keep the original SaveBlock1 array at its historical size. The Johto
// profile appends its second beast to the separately stored SaveBlock3.
#if POLISHED_JOHTO_DEMO && ROAMER_COUNT < 2
#define ACTIVE_ROAMER_SLOTS 2
#else
#define ACTIVE_ROAMER_SLOTS ROAMER_COUNT
#endif

struct Roamer *GetRoamerByIndex(u32 index);
void DeactivateAllRoamers(void);
void InitRoamer(void);
void UpdateLocationHistoryForRoamer(void);
void RoamerMoveToOtherLocationSet(u32 roamerIndex);
void RoamerMove(u32 roamerIndex);
bool8 IsRoamerAt(u32 roamerIndex, u8 mapGroup, u8 mapNum);
void CreateRoamerMonInstance(u32 roamerIndex);
bool8 TryStartRoamerEncounter(void);
bool8 TryStartRoamerEncounterInArea(bool32 ordinaryLand);
void MovePolishedRoamingBeasts(bool32 roamingBattle);
void UpdateRoamerHPStatus(struct Pokemon *mon);
void SetRoamerInactive(u32 roamerIndex);
void GetRoamerLocation(u32 roamerIndex, u8 *mapGroup, u8 *mapNum);
bool8 TryAddRoamer(enum Species species, u8 level);
void MoveAllRoamersToOtherLocationSets(void);
void MoveAllRoamers(void);

extern u8 gEncounteredRoamerIndex;

#endif // GUARD_ROAMER_H
