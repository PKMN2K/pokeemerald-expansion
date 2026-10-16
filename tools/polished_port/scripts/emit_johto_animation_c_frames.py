#!/usr/bin/env python3
"""Generate ROM-resident Johto frame data exclusively from pinned PNG fixtures."""
import argparse
from pathlib import Path
from stage_johto_animation_frames import build_frames
from stage_johto_tile_graphics import build_graphics

ORDER = ("water", "rain_puddle", "rain_water", "flower",
         "whirlpool_1", "whirlpool_2", "whirlpool_3", "whirlpool_4")

def generate():
    groups = build_frames()
    frames = [frame for name in ORDER for frame in groups[name]]
    assert len(frames) == 38 and all(len(frame) == 32 for frame in frames)
    out = [
        "// Generated from pinned Polished Crystal source; do not edit.",
        "#ifndef GUARD_POLISHED_JOHTO_ANIMATION_FRAMES_H",
        "#define GUARD_POLISHED_JOHTO_ANIMATION_FRAMES_H",
        "static const u16 sPolishedJohtoFramePixels[38][16] = {",
    ]
    for frame in frames:
        words = [frame[i] | (frame[i + 1] << 8) for i in range(0, 32, 2)]
        out.append("    {" + ", ".join(f"0x{x:04X}" for x in words) + "},")
    out.append("};")
    out.append("static const u16 *const sPolishedJohtoFrames[38] = {")
    for i in range(38):
        out.append(f"    sPolishedJohtoFramePixels[{i}],")
    out.append("};")
    primary, _, _ = build_graphics()
    scroll = primary[182 * 32:183 * 32]
    assert len(scroll) == 32 and all((byte & 15) in (1, 2, 3, 4)
                                     and (byte >> 4) in (1, 2, 3, 4)
                                     for byte in scroll)
    words = [scroll[i] | (scroll[i + 1] << 8) for i in range(0, 32, 2)]
    out.append("// Authentic VRAM bank 1 tile $35, GBA primary tile 182.")
    out.append("static const u16 sPolishedJohtoInitialScrollTile[16] = {")
    out.append("    " + ", ".join(f"0x{x:04X}" for x in words))
    out += ["};", "#endif", ""]
    return "\n".join(out)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    data = generate()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(data, encoding="utf-8")

if __name__ == "__main__":
    main()
