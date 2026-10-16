#ifndef GUARD_POLISHED_JOHTO_ANIMATION_H
#define GUARD_POLISHED_JOHTO_ANIMATION_H

#ifdef POLISHED_JOHTO_HOST_TEST
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#else
#include "global.h"
#endif

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
// Returns the ROM-resident 38-frame authentic source table (NULL in host tests).
const u16 *const *PolishedJohtoAnimation_AuthenticFrames(void);

// Source frame groups are supplied only after their pinned 4bpp binaries
// are linked. No map or DMA queue function is installed by this API.
typedef void (*PolishedJohtoQueueTile)(const u16 *frame, u16 tileId);
void PolishedJohtoAnimation_DispatchStaged(
    struct PolishedJohtoAnimationState *state,
    const u16 *const *frames, PolishedJohtoQueueTile queue);

#endif
