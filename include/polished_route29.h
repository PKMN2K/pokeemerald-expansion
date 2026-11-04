#ifndef GUARD_POLISHED_ROUTE29_H
#define GUARD_POLISHED_ROUTE29_H
#include "rtc.h"

enum TimeOfDay PolishedRoute29EncounterTime(u32 hour, u32 roll);
bool32 PolishedRoute29FruitAvailable(void);
void PolishedRoute29RefreshFruit(void);
void PolishedRoute29PickFruit(void);
bool32 PolishedFruitAvailable(void);
void PolishedFruitRefreshAll(void);
void PolishedFruitPrepare(void);
void PolishedFruitPick(void);
void PolishedFruitUseMulch(void);

#endif
