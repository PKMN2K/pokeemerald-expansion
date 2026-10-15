#!/usr/bin/env python3
"""Staging-only Johto 22-step animation command interpreter.

The exact dispatch order comes from the pinned Polished Crystal source.
This models the formerly missing 4bpp save / scroll / restore of tile $35;
it does not write VRAM, register an Emerald callback, or modify live maps.
"""
from stage_johto_animation_frames import (
    ACTIONS, ANIMATION_DISPATCH, TARGETS, build_frames, frame_index,
    original_dispatch,
)
from test_johto_scroll_tile_4bpp import scroll_down_4bpp


SCROLL_GBA_TILE = 182
SCROLL_STEPS = (14, 16, 18)


def dispatch_cycle(tiles, timer=0):
    """Return (updated tiles, next 3-bit timer, ordered command log).

    tiles maps GBA primary tile IDs to exactly 32 packed 4bpp bytes.
    At step 12 the displayed tile $35 is copied into a working buffer;
    at steps 14,16,18 that buffer scrolls one pixel row downward;
    at step 20 the buffer is copied back to the displayed tile.
    """
    if not isinstance(timer, int) or timer not in range(8):
        raise ValueError("invalid source animation timer")
    if SCROLL_GBA_TILE not in tiles or len(tiles[SCROLL_GBA_TILE]) != 32:
        raise ValueError("missing original 4bpp scroll tile $35")
    if len(ACTIONS) != 22 or len(original_dispatch()) != 22:
        raise ValueError("source dispatch no longer matches 22 commands")
    groups = build_frames()
    out = dict(tiles)
    working = None
    log = []
    for step, action in enumerate(ACTIONS):
        event = {"step": step, "source_action": action, "timer_before": timer}
        if step in ANIMATION_DISPATCH:
            name = ANIMATION_DISPATCH[step]
            tile_id = TARGETS[name][0] + 129
            frame = frame_index(name, timer)
            out[tile_id] = groups[name][frame]
            event.update(tile=tile_id, animation=name, frame=frame)
        elif step == 11:
            timer = (timer + 1) & 7
        elif step == 12:
            working = bytes(out[SCROLL_GBA_TILE])
            event["buffer_action"] = "copy_tile_to_buffer"
        elif step in SCROLL_STEPS:
            if working is None:
                raise AssertionError("scroll before buffer snapshot")
            working = scroll_down_4bpp(working)
            event["buffer_action"] = "scroll_buffer_down_one_row"
        elif step == 20:
            if working is None:
                raise AssertionError("missing source tile buffer")
            out[SCROLL_GBA_TILE] = working
            event["buffer_action"] = "copy_buffer_to_tile"
        event["timer_after"] = timer
        log.append(event)
    return out, timer, log
