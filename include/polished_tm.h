#ifndef GUARD_POLISHED_TM_H
#define GUARD_POLISHED_TM_H

#include "pokemon.h"

// Source ownership/lesson rules apply only to explicitly migrated machines.
static inline bool32 IsPolishedTM(enum Item item)
{
    return item == ITEM_TM_BRICK_BREAK || item == ITEM_TM_ROOST || item == ITEM_TM70;
}

// Called only after a successful source TM lesson, never on rejection/cancellation.
void PolishedTMFriendship(struct Pokemon *mon);

#endif
