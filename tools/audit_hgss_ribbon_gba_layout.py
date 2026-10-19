#!/usr/bin/env python3
"""Check HGSS Ribbons 240x160 layout against live Emerald UI coordinates.

This is a deterministic geometry audit, NOT a replacement or recreation
of authentic HGSS pixels. No source art is included.
"""
from dataclasses import dataclass

SCREEN_W, SCREEN_H = 240, 160

@dataclass(frozen=True)
class Rect:
    name: str
    x: int
    y: int
    w: int
    h: int

    @property
    def right(self):
        return self.x + self.w

    @property
    def bottom(self):
        return self.y + self.h

    def overlaps(self, other):
        return self.x < other.right and other.x < self.right and self.y < other.bottom and other.y < self.bottom


# Extracted from src/pokenav_ribbons_summary.c on feature/gen4-ui-renderer.
REGIONS = (
    Rect("ribbon_icons_4x2", 88, 32, 128, 64),
    Rect("ribbon_frame_window", 80, 24, 160, 80),
    Rect("pokemon_name_window", 112, 8, 104, 16),
    Rect("ribbon_count_window", 96, 96, 128, 32),
    Rect("mon_list_index_window", 8, 40, 56, 16),
)


def main():
    for rect in REGIONS:
        assert 0 <= rect.x < rect.right <= SCREEN_W, rect
        assert 0 <= rect.y < rect.bottom <= SCREEN_H, rect
        print(f"{rect.name}: x={rect.x}..{rect.right-1}, y={rect.y}..{rect.bottom-1}")
    assert REGIONS[0].overlaps(REGIONS[1])
    assert REGIONS[1].overlaps(REGIONS[3])
    assert not REGIONS[0].overlaps(REGIONS[2])
    assert not REGIONS[0].overlaps(REGIONS[4])
    print("PASS: verified occupancy and expected window overlaps")
    print("NOTE: Pokemon sprite center (40,104) is not a fixed bounding box.")
    print("NOTE: This audit does not verify DS compositing or approve background replacement.")


if __name__ == "__main__":
    main()
