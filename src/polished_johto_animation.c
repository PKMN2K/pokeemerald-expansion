#include "global.h"
#include "polished_johto_animation.h"
#ifndef POLISHED_JOHTO_HOST_TEST
#include "data/tilesets/polished_johto_animation_frames.h"
#endif

// Original source: Rangi42/polishedcrystal 3fa43192379df5c3e7b09a08e4d5d79af4f02f42
// engine/tilesets/tileset_anims.asm. This component is intentionally not
// connected to Emerald's primary/secondary callback tables or VRAM yet.
// A registered tileset can eventually call Step once per dispatch tick
// and queue authentic frame data using Emerald's existing DMA pipeline.

void PolishedJohtoAnimation_Reset(struct PolishedJohtoAnimationState *state, const u8 *initialTile)
{
    u32 i;

    if (state == NULL || initialTile == NULL)
        return;
    state->dispatchStep = 0;
    state->timer = 0;
    for (i = 0; i < 32; i++)
    {
        state->scrollTile[i] = initialTile[i];
        state->scrollBuffer[i] = 0;
    }
}

static void ScrollBufferDown(struct PolishedJohtoAnimationState *state)
{
    u8 bottom[4];
    u32 i;

    for (i = 0; i < 4; i++)
        bottom[i] = state->scrollBuffer[28 + i];
    for (i = 28; i > 0; i -= 4)
    {
        state->scrollBuffer[i] = state->scrollBuffer[i - 4];
        state->scrollBuffer[i + 1] = state->scrollBuffer[i - 3];
        state->scrollBuffer[i + 2] = state->scrollBuffer[i - 2];
        state->scrollBuffer[i + 3] = state->scrollBuffer[i - 1];
    }
    for (i = 0; i < 4; i++)
        state->scrollBuffer[i] = bottom[i];
}

void PolishedJohtoAnimation_Step(struct PolishedJohtoAnimationState *state)
{
    u32 i;

    if (state == NULL)
        return;
    switch (state->dispatchStep)
    {
    case 11: // StandingTileFrame8: advance the 3-bit counter only here
        state->timer = (state->timer + 1) & 7;
        break;
    case 12: // WriteTileToBuffer: copy source 4bpp tile $35
        for (i = 0; i < 32; i++)
            state->scrollBuffer[i] = state->scrollTile[i];
        break;
    case 14:
    case 16:
    case 18: // three original one-row downward rotations
        ScrollBufferDown(state);
        break;
    case 20: // ReadTileFromBuffer: restore to target 4bpp tile $35
        for (i = 0; i < 32; i++)
            state->scrollTile[i] = state->scrollBuffer[i];
        break;
    }
    state->dispatchStep++;
    if (state->dispatchStep == 22)
        state->dispatchStep = 0;
}

// 0 water, 1 puddle, 2 rain water, 3 flower, 4..7 whirlpool pieces.
// Frames are indexes into the separately pinned, authentic source image
// arrays. This function deliberately does not invent graphics or use DMA.
u8 PolishedJohtoAnimation_Frame(u8 stream, u8 timer)
{
    timer &= 7;
    switch (stream)
    {
    case 0: return (timer >> 1) & 3;
    case 1:
    case 2: return timer;
    case 3: return (timer >> 1) & 1;
    case 4:
    case 5:
    case 6:
    case 7: return timer & 3;
    default: return 0;
    }
}

// Dispatches only verified source tile destinations. Each stream is stored
// contiguously in the caller's authenticated 4bpp frame table:
// water(4), rain puddle(8), rain water(8), flower(2), whirlpool 1..4(4 each).
// The queue callback can be Emerald's guarded DMA bridge, but no runtime
// registration occurs here. Frame storage and state must remain alive until
// TransferTilesetAnimsBuffer drains the deferred transfer queue.
void PolishedJohtoAnimation_DispatchStaged(
    struct PolishedJohtoAnimationState *state,
    const u16 *const *frames, PolishedJohtoQueueTile queue)
{
    static const u8 sSteps[] = {0, 1, 2, 4, 5, 6, 7, 8, 9, 10};
    static const u8 sStreams[] = {0, 1, 2, 3, 4, 5, 6, 7, 1, 2};
    static const u8 sOffsets[] = {0, 4, 12, 20, 22, 26, 30, 34};
    static const u16 sDestinations[] = {149, 157, 158, 132, 178, 179, 194, 195};
    u32 i;

    if (state == NULL)
        return;

    // The pinned frame table is optional until an authentic Johto tileset
    // is registered. Never transfer uninitialized or placeholder graphics.
    if (frames != NULL && queue != NULL)
    {
        for (i = 0; i < 10; i++)
        {
            if (state->dispatchStep == sSteps[i])
            {
                u8 stream = sStreams[i];
                const u16 *frame = frames[sOffsets[stream] +
                    PolishedJohtoAnimation_Frame(stream, state->timer)];
                if (frame != NULL)
                    queue(frame, sDestinations[stream]);
                break;
            }
        }
    }

    PolishedJohtoAnimation_Step(state);

    if (frames != NULL && queue != NULL && state->dispatchStep == 21)
        queue((const u16 *)state->scrollTile, 182);
}

const u16 *const *PolishedJohtoAnimation_AuthenticFrames(void)
{
#ifdef POLISHED_JOHTO_HOST_TEST
    return NULL;
#else
    return sPolishedJohtoFrames;
#endif
}
