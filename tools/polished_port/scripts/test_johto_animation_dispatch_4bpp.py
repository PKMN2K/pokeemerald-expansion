#!/usr/bin/env python3
"""Regression for source-order Johto 4bpp animation staging dispatcher."""
from stage_johto_animation_dispatch_4bpp import dispatch_cycle, SCROLL_GBA_TILE
from stage_johto_animation_frames import ANIMATION_DISPATCH, TARGETS, frame_index


def main():
    initial = bytes(range(32))
    tiles = {SCROLL_GBA_TILE: initial}
    for cycle in range(8):
        tiles, timer, events = dispatch_cycle(tiles, cycle)
        assert timer == (cycle + 1) % 8
        assert len(events) == 22
        assert [e["step"] for e in events] == list(range(22))
        assert events[11]["timer_before"] == cycle
        assert events[11]["timer_after"] == timer
        assert events[12]["buffer_action"] == "copy_tile_to_buffer"
        for step in (14, 16, 18):
            assert events[step]["buffer_action"] == "scroll_buffer_down_one_row"
        assert events[20]["buffer_action"] == "copy_buffer_to_tile"
        for step, name in ANIMATION_DISPATCH.items():
            assert events[step]["animation"] == name
            assert events[step]["frame"] == frame_index(name, cycle)
            assert events[step]["tile"] == TARGETS[name][0] + 129
        rows = (3 * (cycle + 1)) % 8
        expected = initial[-4 * rows:] + initial[:-4 * rows] if rows else initial
        assert tiles[SCROLL_GBA_TILE] == expected
    assert tiles[SCROLL_GBA_TILE] == initial
    for value in (-1, 8):
        try:
            dispatch_cycle({SCROLL_GBA_TILE: initial}, value)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid timer accepted")
    print("PASS: eight complete authentic 22-command cycles")
    print("PASS: tile $35 rolls down three rows each source cycle and returns after eight")
    print("PASS: source timer, tile destinations, and frame order")
    print("STAGED ONLY: no Emerald callback or registered map layout")


if __name__ == "__main__":
    main()
