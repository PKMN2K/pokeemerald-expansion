#ifndef GUARD_POLISHED_ESCAPE_H
#define GUARD_POLISHED_ESCAPE_H

bool32 UsesPolishedEscapeRules(enum BattlerId battler);
u32 PolishedEscapeThreshold(u16 playerSpeed, u16 enemySpeed, u8 previousAttempts);
u8 PolishedRunPreflight(enum BattlerId battler);
bool32 TryPolishedRun(enum BattlerId battler);
bool32 CanPolishedEnemyFlee(enum BattlerId battler);
bool32 CanPolishedRoamerChooseFlee(enum BattlerId battler);
u32 PolishedRoamerSavedStatus(u32 status);

#endif
