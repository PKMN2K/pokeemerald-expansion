#ifndef GUARD_BATTLE_SWITCH_IN
#define GUARD_BATTLE_SWITCH_IN

#include "constants/battle_switch_in.h"

bool32 DoSwitchInEvents(void);
void PolishedBeginIntimidate(enum BattlerId battler);
bool32 PolishedIntimidateItems(void);
void PolishedGasExit(bool32 active);
void ResetPolishedEntryItems(void);

#endif // GUARD_BATTLE_SWITCH_IN
