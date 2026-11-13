#include "global.h"
#include "polished_metronome.h"
#include "constants/polished_metronome_struggle.h"

#include "data/polished_metronome_moves.h"

enum Move PolishedMetronomeMoveFromRoll(u32 roll, const enum Move *knownMoves)
{
    if (roll >= ARRAY_COUNT(sPolishedMetronomeMoves))
        return MOVE_NONE;

    enum Move move = sPolishedMetronomeMoves[roll];
    if (move == MOVE_NONE)
        return MOVE_NONE;
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        if (knownMoves[slot] == move)
            return MOVE_NONE;
    return move;
}
