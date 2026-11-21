#ifndef GUARD_POLISHED_FRIENDSHIP_H
#define GUARD_POLISHED_FRIENDSHIP_H

#include "pokemon.h"
#include "constants/polished_happiness.h"

bool32 UsesPolishedFriendship(struct Pokemon *mon);
bool32 UsesPolishedBattleItemFriendship(u32 battler, enum Item item);
u32 PolishedFriendshipAfterEvent(u8 friendship, u32 event, bool32 sootheBell, bool32 luxuryBall);
bool32 PolishedTryAdjustFriendship(struct Pokemon *mon, u32 event);
bool32 PolishedIsBossTrainer(u32 trainer);

#endif
