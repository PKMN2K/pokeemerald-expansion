#!/usr/bin/env python3
"""Losslessly adapt the verified HGSS summary card to feeding prompts."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from make_hgss_ribbon_summary import ASSETS, validate_assets

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "graphics/gen4_ui/hgss_pokeblock_use"


def build():
    validate_assets()
    raw = (ASSETS / "summary_bg.4bpp").read_bytes()
    screen = struct.unpack("<1024H", (ASSETS / "summary_bg.bin").read_bytes())

    def card(width, height):
        tiles = []
        for y in range(height):
            for x in range(width):
                sx = 0 if x == 0 else 9 if x == width - 1 else 2
                sy = 4 if y == 0 else 7 if y == height - 1 else 5
                attr = screen[sy * 32 + sx]
                assert attr >> 12 == 1
                pixels = []
                for yy in range(8):
                    for xx in range(8):
                        px = 7 - xx if attr & 0x400 else xx
                        py = 7 - yy if attr & 0x800 else yy
                        value = raw[(attr & 1023) * 32 + py * 4 + px // 2]
                        pixels.append((value >> (4 * (px % 2))) & 15)
                tiles.append(bytes(pixels[i] | pixels[i + 1] << 4 for i in range(0, 64, 2)))
        return b"".join(tiles)

    outputs = {"footer.4bpp": card(30, 4), "confirmation.4bpp": card(8, 6)}
    manifest = {
        "source": "../hgss_ribbons/summary_bg.source.json",
        "source_manifest_sha256": hashlib.sha256((ASSETS / "summary_bg.source.json").read_bytes()).hexdigest(),
        "adaptation": "Original party-card corners, edges and interiors; complete 8x8 cells, native colors, no repainting or scaling",
        "footer_tiles": [30, 4], "confirmation_tiles": [8, 6], "palette_bank": 1,
        "background": "../hgss_condition/source.json",
        "sheen": "../hgss_storage/verified/shiny_indicator.json",
        "indicators": "Original HGSS Pokédex list arrows and pointer, via make_hgss_pokegear_list_arrows.py",
        "outputs": {name: hashlib.sha256(data).hexdigest() for name, data in outputs.items()},
    }
    outputs["source.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    return outputs


def validate():
    for name, data in build().items():
        assert (OUT / name).read_bytes() == data, f"Stale HGSS feeding asset: {name}"
    print("Verified original HGSS feeding footer and confirmation card")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.check:
        validate()
    else:
        OUT.mkdir(parents=True, exist_ok=True)
        for name, data in build().items():
            (OUT / name).write_bytes(data)
