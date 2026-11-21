#ifndef GUARD_POLISHED_LIFECYCLE_H
#define GUARD_POLISHED_LIFECYCLE_H

#include "global.h"
#include "constants/battle.h"

bool32 UsesPolishedLifecycle(void);
void ResetPolishedEndTurn(void);
bool32 PolishedEndTurnDefersFaints(void);
bool32 PolishedEndTurnDefersItems(void);
bool32 PolishedQueueEndTurnFaint(enum BattlerId battler);
void SortPolishedBattlersBySpeed(enum BattlerId *order);
void PolishedBeginEntryHazards(void);
bool32 PolishedResolveEntryFaints(void);
void PolishedOrderReplacements(void);
bool32 PolishedNextReplacement(void);
bool32 PolishedHandleUnnerveExit(enum BattlerId battler);
void PolishedRecordForcedEntry(enum BattlerId battler, bool32 forced);
bool32 PolishedIsForcedEntry(enum BattlerId battler);

#endif
