#ifndef GUARD_POLISHED_LIFECYCLE_H
#define GUARD_POLISHED_LIFECYCLE_H

#include "global.h"
#include "constants/battle.h"

enum PolishedSwitchKind
{
    POLISHED_SWITCH_NONE,
    POLISHED_SWITCH_PIVOT,
    POLISHED_SWITCH_BATON_PASS,
    POLISHED_SWITCH_FORCED,
    POLISHED_SWITCH_BUTTON,
    POLISHED_SWITCH_CARD,
    POLISHED_SWITCH_PACK,
};
void PolishedReserveSwitch(enum PolishedSwitchKind kind, enum BattlerId battler);
bool32 PolishedExecutingDeferredSwitch(void);
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
bool32 UsesPolishedActionRules(enum Move move);
void PolishedBeginActionFaints(enum Move move);
bool32 PolishedResolveActionFaints(void);
void PolishedPrioritizeActionFaint(enum BattlerId battler);

#endif
