#ifndef GUARD_POLISHED_JOHTO_HEADBUTT_H
#define GUARD_POLISHED_JOHTO_HEADBUTT_H

#include "global.h"

enum JohtoHeadbuttEncounterResult
{
    JOHTO_HEADBUTT_INVALID_INPUT,
    JOHTO_HEADBUTT_NO_ENCOUNTER_SET,
    JOHTO_HEADBUTT_NO_ENCOUNTER,
    JOHTO_HEADBUTT_ENCOUNTER,
    JOHTO_HEADBUTT_SPECIES_UNAVAILABLE,
};

struct JohtoHeadbuttEncounter
{
    u16 species; // Expansion species ID, including regional form.
    u8 level;
};

// Dormant data/selection API. Group, number and facing coordinates are from
// pinned Polished Crystal, NEVER Emerald's current map or padded map grid.
// Callers must explicitly translate a future registered map to these source IDs.
// badgeCount is the source 0..16 progression; rolls are uniform 0..9 and 0..99.
// No RNG, save data, party, scripts or battles are touched. Only ENCOUNTER writes
// out; missing maps, unsuccessful rolls and disabled species never substitute.
enum JohtoHeadbuttEncounterResult JohtoHeadbuttSelectEncounterFromSourceMap(
    u8 sourceMapGroup, u8 sourceMapNumber, u8 facingX, u8 facingY,
    u16 trainerId, u8 badgeCount, u8 battleRoll, u8 slotRoll,
    struct JohtoHeadbuttEncounter *out);

#endif // GUARD_POLISHED_JOHTO_HEADBUTT_H
