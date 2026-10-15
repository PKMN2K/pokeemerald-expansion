#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "polished_johto_animation.h"

static void RotateRows(u8 *tile)
{
    u8 last[4];
    memcpy(last, tile + 28, 4);
    memmove(tile + 4, tile, 28);
    memcpy(tile, last, 4);
}

static unsigned queuedCount;
static u16 lastQueuedTile;
static const u16 *lastQueuedFrame;

static void RecordTile(const u16 *frame, u16 tileId)
{
    assert(frame != NULL);
    queuedCount++;
    lastQueuedTile = tileId;
    lastQueuedFrame = frame;
}

int main(void)
{
    struct PolishedJohtoAnimationState state;
    u8 initial[32], expected[32];
    u32 i, cycle, step, stream, timer;
    const u8 expectedFrames[8][8] = {
        {0, 0, 1, 1, 2, 2, 3, 3},
        {0, 1, 2, 3, 4, 5, 6, 7},
        {0, 1, 2, 3, 4, 5, 6, 7},
        {0, 0, 1, 1, 0, 0, 1, 1},
        {0, 1, 2, 3, 0, 1, 2, 3},
        {0, 1, 2, 3, 0, 1, 2, 3},
        {0, 1, 2, 3, 0, 1, 2, 3},
        {0, 1, 2, 3, 0, 1, 2, 3}
    };

    for (i = 0; i < 32; i++)
        initial[i] = i;
    memcpy(expected, initial, sizeof(expected));
    memset(&state, 0xAA, sizeof(state));
    PolishedJohtoAnimation_Reset(&state, initial);
    assert(state.dispatchStep == 0 && state.timer == 0);
    assert(memcmp(state.scrollTile, initial, 32) == 0);
    for (i = 0; i < 32; i++)
        assert(state.scrollBuffer[i] == 0);

    for (cycle = 0; cycle < 8; cycle++)
    {
        for (step = 0; step < 22; step++)
        {
            assert(state.dispatchStep == step);
            assert(state.timer == (cycle + (step > 11)) % 8);
            PolishedJohtoAnimation_Step(&state);
            if (step == 12)
                assert(memcmp(state.scrollBuffer, expected, 32) == 0);
            if (step == 14 || step == 16 || step == 18)
            {
                RotateRows(expected);
                assert(memcmp(state.scrollBuffer, expected, 32) == 0);
            }
            if (step == 20)
                assert(memcmp(state.scrollTile, expected, 32) == 0);
        }
        assert(state.dispatchStep == 0);
        assert(state.timer == (cycle + 1) % 8);
        assert(memcmp(state.scrollTile, expected, 32) == 0);
    }
    assert(memcmp(state.scrollTile, initial, 32) == 0);
    for (stream = 0; stream < 8; stream++)
        for (timer = 0; timer < 8; timer++)
        {
            assert(PolishedJohtoAnimation_Frame(stream, timer) == expectedFrames[stream][timer]);
            assert(PolishedJohtoAnimation_Frame(stream, timer + 8) == expectedFrames[stream][timer]);
        }
    assert(PolishedJohtoAnimation_Frame(255, 0) == 0);
    PolishedJohtoAnimation_Step(NULL);
    PolishedJohtoAnimation_Reset(NULL, initial);
    PolishedJohtoAnimation_Reset(&state, NULL);
    // Exercise the opt-in transfer callback without touching DMA/VRAM.
    {
        const u16 *frames[38];
        u16 payload[38][16];
        const u16 destinations[10] = {149, 157, 158, 132, 178, 179, 194, 195, 157, 158};
        const u8 steps[10] = {0, 1, 2, 4, 5, 6, 7, 8, 9, 10};
        unsigned emitted = 0, event = 0;
        for (i = 0; i < 38; i++)
            frames[i] = payload[i];
        PolishedJohtoAnimation_Reset(&state, initial);
        queuedCount = 0;
        for (step = 0; step < 22; step++)
        {
            unsigned prior = queuedCount;
            PolishedJohtoAnimation_DispatchStaged(&state, frames, RecordTile);
            if (event < 10 && step == steps[event])
            {
                assert(queuedCount == prior + 1);
                assert(lastQueuedTile == destinations[event]);
                event++;
                emitted++;
            }
            else if (step == 20)
            {
                assert(queuedCount == prior + 1);
                assert(lastQueuedTile == 182);
                assert(lastQueuedFrame == (const u16 *)state.scrollTile);
                emitted++;
            }
            else
                assert(queuedCount == prior);
        }
        assert(emitted == 11 && event == 10);
        queuedCount = 0;
        PolishedJohtoAnimation_Reset(&state, initial);
        PolishedJohtoAnimation_DispatchStaged(&state, NULL, RecordTile);
        assert(queuedCount == 0);
    }
    puts("PASS: native C 176-step dispatcher, buffers, and 64 authentic frame selectors");
    return 0;
}
