#!/usr/bin/env python3
"""Source-parity tests for nine pinned Johto animation PNGs and 22-step timer."""
import copy
import json
from pathlib import Path
import tempfile

from stage_johto_animation_frames import (
    ACTIONS, ANIMATION_DISPATCH, PINNED_PNG_BLOBS, PNG_FIXTURE,
    SOURCE_IMAGES, TARGETS, build_frames, frame_index, load_images,
    original_dispatch, stage, trace_source_cycles, verify_source_timing,
)
from stage_johto_tile_graphics import (
    gba_tile_id, load_source_metatile_tables, priority_overlay_tile_ids,
)
from test_johto_animation_schedule import parse_schedule


def rejects(callback):
    try:
        callback()
    except (ValueError, KeyError):
        return
    raise AssertionError("unexpected Johto animation/source input accepted")


def main():
    assert verify_source_timing()
    commands = original_dispatch()
    assert len(commands) == len(ACTIONS) == 22
    assert commands == parse_schedule(
        (PNG_FIXTURE.parent / "johto_outdoor_animation_schedule.asm")
        .read_text(encoding="utf-8")
    )
    assert len(ANIMATION_DISPATCH) == 10
    assert set(ANIMATION_DISPATCH) == {0, 1, 2, 4, 5, 6, 7, 8, 9, 10}

    originals = load_images()
    assert len(originals) == len(PINNED_PNG_BLOBS) == len(SOURCE_IMAGES) == 9
    assert {name: len(originals[name]) for name, *_ in SOURCE_IMAGES} == {
        "water": 4, "rain_puddle": 8, "rain_water": 8,
        "flower_1": 1, "flower_2": 1,
        "whirlpool_1": 4, "whirlpool_2": 4,
        "whirlpool_3": 4, "whirlpool_4": 4,
    }
    groups = build_frames()
    assert len(groups) == 8
    assert sum(len(frames) for frames in groups.values()) == 38
    assert groups["flower"] == originals["flower_1"] + originals["flower_2"]
    for name, tiles in groups.items():
        assert name in TARGETS
        assert len(tiles) == TARGETS[name][1]
        assert all(len(tile) == 32 for tile in tiles)
        assert all(all((b & 15) in (1, 2, 3, 4)
                       and (b >> 4) in (1, 2, 3, 4) for b in tile)
                   for tile in tiles)

    expected_gba_destinations = {
        "water": 149, "rain_puddle": 157, "rain_water": 158,
        "flower": 132, "whirlpool_1": 178, "whirlpool_2": 179,
        "whirlpool_3": 194, "whirlpool_4": 195,
    }
    assert {name: gba_tile_id(tile, 1) for name, (tile, _, _) in TARGETS.items()} == (
        expected_gba_destinations
    )
    source_meta, source_attr = load_source_metatile_tables()
    priority_tiles = set(priority_overlay_tile_ids(source_meta, source_attr))
    assert not priority_tiles.intersection(expected_gba_destinations.values())
    assert len(priority_tiles) == 12

    # The source increments wTileAnimationTimer ONLY at dispatch command
    # 11; it wraps every 8 complete 22-command cycles. Each original
    # selector is tied to this source timer, not host GBA VBlank frames.
    trace = trace_source_cycles(8)
    assert len(trace) == 8 * 22 == 176
    for cycle in range(8):
        items = trace[cycle * 22: (cycle + 1) * 22]
        assert [(x["source_cycle"], x["dispatch_step"]) for x in items] == [
            (cycle, step) for step in range(22)
        ]
        assert items[0]["timer_before_step"] == cycle
        assert items[10]["timer_after_step"] == cycle
        assert items[11]["source_action"] == "StandingTileFrame8"
        assert items[11]["timer_before_step"] == cycle
        assert items[11]["timer_after_step"] == (cycle + 1) % 8
        assert items[21]["source_action"] == "DoneTileAnimation"
        assert items[21]["timer_after_step"] == (cycle + 1) % 8
        for step, name in ANIMATION_DISPATCH.items():
            item = items[step]
            assert item["source_action"] == ACTIONS[step]
            assert item["animation"] == name
            assert item["frame_index"] == frame_index(name, cycle)
            assert item["gba_primary_tile_id"] == expected_gba_destinations[name]
        assert [items[step]["source_action"] for step in (14, 16, 18)] == (
            ["ScrollTileDown"] * 3
        )
        assert items[12]["pending_scroll_buffer_gba_tile"] == 182
        assert items[20]["pending_scroll_buffer_gba_tile"] == 182
    for name in TARGETS:
        expected = {
            "water": [0, 0, 1, 1, 2, 2, 3, 3],
            "rain_puddle": list(range(8)),
            "rain_water": list(range(8)),
            "flower": [0, 0, 1, 1, 0, 0, 1, 1],
            "whirlpool_1": [0, 1, 2, 3, 0, 1, 2, 3],
            "whirlpool_2": [0, 1, 2, 3, 0, 1, 2, 3],
            "whirlpool_3": [0, 1, 2, 3, 0, 1, 2, 3],
            "whirlpool_4": [0, 1, 2, 3, 0, 1, 2, 3],
        }[name]
        assert [frame_index(name, t) for t in range(8)] == expected

    with tempfile.TemporaryDirectory(prefix="johto-anim-") as tmp:
        output = Path(tmp)
        report = stage(output)
        assert report["status"] == "authentic_4bpp_frames_staged_not_live"
        assert report["source_total_animation_frames"] == 38
        assert not report["emerald_callback_registered"]
        assert not report["johto_map_registered"]
        assert not report["ready_for_live_registration"]
        assert report["scroll_buffer_tile"]["gba_primary_tile_id"] == 182
        assert not report["scroll_buffer_tile"]["runtime_4bpp_scroll_verified"]
        assert report["pinned_png_files"] == 9
        for name, frames in groups.items():
            original, count, shift = TARGETS[name]
            entry = report["animations"][name]
            assert entry["gba_primary_tile_id"] == expected_gba_destinations[name]
            assert entry["source_vram_tile"] == original
            assert entry["source_frame_index_over_8_timer_phases"] == [
                frame_index(name, t) for t in range(8)
            ]
            assert entry["gba_4bpp_staged_bytes"] == count * 32
            assert (output / entry["filename"]).read_bytes() == b"".join(frames)
        assert json.loads((output / "johto.animations.dispatch.staging.json")
                          .read_text(encoding="utf-8")) == trace
        assert len(list(output.glob("*.4bpp"))) == 8

    # Regression: reject edited source bytes, changed source paths and
    # invented animation timer values. Source fixtures remain byte-identical
    # to the original pinned Polished Crystal Git blobs.
    for n in (-1, 8, 9, 256):
        rejects(lambda n=n: frame_index("water", n))
    rejects(lambda: frame_index("invented", 0))
    rejects(lambda: trace_source_cycles(0))
    rejects(lambda: trace_source_cycles(17))
    doc = json.loads(PNG_FIXTURE.read_text(encoding="utf-8"))
    with tempfile.TemporaryDirectory(prefix="johto-corrupt-") as tmp:
        p = Path(tmp) / "changed.json"
        mutant = copy.deepcopy(doc)
        mutant["assets"]["water"]["source_path"] = "gfx/tilesets/water/fake.png"
        p.write_text(json.dumps(mutant), encoding="utf-8")
        rejects(lambda: load_images(p))
        mutant = copy.deepcopy(doc)
        mutant["assets"]["water"]["png_base64"] = "AAAA"
        p.write_text(json.dumps(mutant), encoding="utf-8")
        rejects(lambda: load_images(p))

    print("PASS: all nine original 2-bit Johto PNG blobs hash-match pinned upstream")
    print("PASS: 38 authentic 8x8 frames staged as unchanged-color GBA 4bpp")
    print("PASS: source water/rain/flower/whirlpool destination tile IDs preserved")
    print("PASS: 176 dispatch commands simulate eight original 22-step cycles")
    print("PASS: original 3-bit timer frame selections and per-loop increment")
    print("PASS: none of eight animated targets overlaps 12 foreground priority masks")
    print("REVIEW: no Emerald callback; source 2bpp scroll buffer not ported yet")


if __name__ == "__main__":
    main()
