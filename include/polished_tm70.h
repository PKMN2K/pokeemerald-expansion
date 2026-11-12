#ifndef GUARD_POLISHED_TM70_H
#define GUARD_POLISHED_TM70_H

#include "pokemon.h"

// Called only after a successful TM70 lesson, never on rejection/cancellation.
void PolishedTM70Friendship(struct Pokemon *mon);

#endif
