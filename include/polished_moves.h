#ifndef GUARD_POLISHED_MOVES_H
#define GUARD_POLISHED_MOVES_H

#include "config_changes.h"
#include "constants/moves.h"
#include "constants/polished_move_pp.h"

extern const u8 gPolishedMovePP[MOVES_COUNT];

static ALWAYS_INLINE bool32 PolishedMovePPEnabled(void)
{
    bool32 enabled = B_POLISHED_MOVE_PP;
#if TESTING
    if (gConfigChangesTestOverride != NULL)
        enabled = gConfigChangesTestOverride->polishedMovePP;
#endif
    return enabled;
}

// Zero selects native data, including optional Expansion and Z/Max moves.
static ALWAYS_INLINE u32 PolishedGetMovePP(enum Move move)
{
    if (!PolishedMovePPEnabled() || (u32)move >= MOVES_COUNT)
        return 0;
    return gPolishedMovePP[move];
}

#endif
