#ifndef GUARD_POLISHED_METRONOME_H
#define GUARD_POLISHED_METRONOME_H

#include "constants/moves.h"

// Pure source adapter: MOVE_NONE requests another draw. Known slots are the
// current battle moves, including zero-PP, disabled and transformed moves.
enum Move PolishedMetronomeMoveFromRoll(u32 roll, const enum Move *knownMoves);

#endif
