#ifndef GUARD_POLISHED_BATTLE_H
#define GUARD_POLISHED_BATTLE_H

#include "config_changes.h"
#include "constants/polished_return_power.h"

// Avoid the generic configuration switch in repeated native AI damage checks.
static ALWAYS_INLINE bool32 PolishedReturnEnabled(void)
{
    bool32 enabled = B_POLISHED_RETURN;
#if TESTING
    if (gConfigChangesTestOverride != NULL)
        enabled = gConfigChangesTestOverride->polishedReturn;
#endif
    return enabled;
}

static inline u32 PolishedReturnPower(u32 friendship)
{
    return min(POLISHED_RETURN_MAX_POWER, friendship * POLISHED_RETURN_MAX_POWER / POLISHED_RETURN_MAX_HAPPINESS);
}

#endif
