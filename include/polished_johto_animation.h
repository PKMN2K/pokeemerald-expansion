#ifndef GUARD_POLISHED_JOHTO_ANIMATION_H
#define GUARD_POLISHED_JOHTO_ANIMATION_H

#include "global.h"

// Source tileset animation is opt-in. No map, tileset callback, DMA or VRAM
// writes are registered here.
struct PolishedJohtoAnimationState
{
    u8 dispatchStep;
    u8 timer;
    u8 scrollTile[32];
    u8 scrollBuffer[32];
};

void PolishedJohtoAnimation_Reset(struct PolishedJohtoAnimationState *state, const u8 *initialTile);
void PolishedJohtoAnimation_Step(struct PolishedJohtoAnimationState *state);
u8 PolishedJohtoAnimation_Frame(u8 stream, u8 timer);

#endif
