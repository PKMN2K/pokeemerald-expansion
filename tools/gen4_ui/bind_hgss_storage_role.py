#!/usr/bin/env python3
"""
Bind a visually verified HGSS storage archive composition to a semantic role.

This is the bridge between the neutral /a/0/1/9 gallery and pokeemerald-side
asset integration. It deliberately requires explicit member IDs: semantic roles
must be chosen by a human after viewing the authentic HGSS gallery, never
guessed from archive adjacency.

Example:
    python tools/gen4_ui/bind_hgss_storage_role.py \
        build/hgss_storage \
        --role party_panel \
        --ncgr 14 --nclr 4 --nscr 8 \
        --output graphics/gen4_ui/hgss_storage/verified/party_panel

The command writes:
    <output>.png
    <output>.json

The JSON sidecar records the archive hash, member IDs, member hashes, dimensions,
crop, and source path so every checked-in conversion can be traced back to the
user-provided HeartGold/SoulSilver source archive without storing that ROM/NARC.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import render_hgss_nitro as nitro


# These are static NCGR+NCLR+NSCR composition roles only.
# "box_grid" is intentionally absent because HGSS uses the wallpaper itself for
# the normal box area. "choose_box" is intentionally absent because HGSS box
# selection/navigation is a dynamic runtime composition rather than a standalone
# background screen. See graphics/gen4_ui/hgss_storage/verified/choose_box_native.json.
ALLOWED_ROLES = {
    "box_header",
    "box_arrows",
    "party_panel",
    "context_menu",
    "message_window",
    "yes_no",
    "cursor",
    "markings_menu",
    "mon_info_panel",
}


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SystemExit(f"Required pipeline output is missing: {path}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON in {path}: {exc}") from exc


def member_by_index(manifest: dict, index: int) -> dict:
    for member in manifest.get("members", []):
        if member.get("index") == index:
            return member
    raise SystemExit(f"Member {index} is not present in manifest.json")


def require_type(manifest: dict, index: int, expected: str) -> dict:
    member = member_by_index(manifest, index)
    actual = member.get("type")
    if actual != expected:
        raise SystemExit(
            f"Member {index} is {actual!r}; role binding expected {expected}"
        )
    return member


def find_member_file(root: Path, index: int, kind: str) -> Path:
    suffixes = {
        "NCGR": ".ncgr",
        "NCLR": ".nclr",
        "NSCR": ".nscr",
    }
    expected = root / "members" / f"{index:03d}_{kind.lower()}{suffixes[kind]}"
    if not expected.exists():
        raise SystemExit(
            f"Decoded {kind} member file is missing: {expected}\n"
            "Run preview_hgss_storage.py first."
        )
    return expected


def crop_rgb(
    rgb: bytes,
    width: int,
    height: int,
    crop: tuple[int, int, int, int] | None,
) -> tuple[int, int, bytes]:
    if crop is None:
        return width, height, rgb

    x, y, crop_width, crop_height = crop
    if x < 0 or y < 0 or crop_width <= 0 or crop_height <= 0:
        raise SystemExit("--crop requires non-negative x/y and positive width/height")
    if x + crop_width > width or y + crop_height > height:
        raise SystemExit(
            f"Crop {crop_width}x{crop_height}+{x}+{y} exceeds "
            f"rendered image {width}x{height}"
        )

    out = bytearray(crop_width * crop_height * 3)
    src_stride = width * 3
    dst_stride = crop_width * 3
    for row in range(crop_height):
        src_start = ((y + row) * width + x) * 3
        dst_start = row * dst_stride
        out[dst_start : dst_start + dst_stride] = rgb[
            src_start : src_start + dst_stride
        ]
    return crop_width, crop_height, bytes(out)


def unique_colors(rgb: bytes) -> int:
    return len(
        {
            (rgb[i], rgb[i + 1], rgb[i + 2])
            for i in range(0, len(rgb), 3)
        }
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Bind verified HGSS storage members to a named UI role"
    )
    parser.add_argument(
        "pipeline_dir",
        type=Path,
        help="Output directory created by preview_hgss_storage.py",
    )
    parser.add_argument(
        "--role",
        required=True,
        choices=sorted(ALLOWED_ROLES),
        help="Semantic storage UI role being verified",
    )
    parser.add_argument("--ncgr", required=True, type=int, help="Verified NCGR member index")
    parser.add_argument("--nclr", required=True, type=int, help="Verified NCLR member index")
    parser.add_argument("--nscr", required=True, type=int, help="Verified NSCR member index")
    parser.add_argument(
        "--crop",
        nargs=4,
        type=int,
        metavar=("X", "Y", "WIDTH", "HEIGHT"),
        help="Optional pixel crop applied after exact NSCR reconstruction",
    )
    parser.add_argument(
        "--output",
        required=True,
        type=Path,
        help="Output path without extension",
    )
    args = parser.parse_args()

    manifest_path = args.pipeline_dir / "manifest.json"
    index_path = args.pipeline_dir / "index.json"
    manifest = load_json(manifest_path)
    gallery_index = load_json(index_path)

    if manifest.get("archive_sha256") != gallery_index.get("archive_sha256"):
        raise SystemExit(
            "manifest.json and index.json do not describe the same HGSS archive"
        )
    if manifest.get("member_count") != 87:
        raise SystemExit(
            f"Expected authentic HGSS /a/0/1/9 to contain 87 members; "
            f"manifest reports {manifest.get('member_count')}"
        )

    ncgr_record = require_type(manifest, args.ncgr, "NCGR")
    nclr_record = require_type(manifest, args.nclr, "NCLR")
    nscr_record = require_type(manifest, args.nscr, "NSCR")

    ncgr_path = find_member_file(args.pipeline_dir, args.ncgr, "NCGR")
    nclr_path = find_member_file(args.pipeline_dir, args.nclr, "NCLR")
    nscr_path = find_member_file(args.pipeline_dir, args.nscr, "NSCR")

    ncgr = nitro.parse_ncgr(ncgr_path)
    nclr = nitro.parse_nclr(nclr_path)
    nscr = nitro.parse_nscr(nscr_path)

    width, height, rgb = nitro.render_screen(ncgr, nclr, nscr)
    crop = tuple(args.crop) if args.crop is not None else None
    out_width, out_height, out_rgb = crop_rgb(rgb, width, height, crop)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    png_path = args.output.with_suffix(".png")
    metadata_path = args.output.with_suffix(".json")
    nitro.write_rgb_png(png_path, out_width, out_height, out_rgb)

    metadata = {
        "role": args.role,
        "source_archive": manifest.get("source_archive"),
        "source_kind": manifest.get("source_kind"),
        "archive_sha256": manifest.get("archive_sha256"),
        "members": {
            "ncgr": {
                "index": args.ncgr,
                "sha256_decoded": ncgr_record.get("sha256_decoded"),
            },
            "nclr": {
                "index": args.nclr,
                "sha256_decoded": nclr_record.get("sha256_decoded"),
            },
            "nscr": {
                "index": args.nscr,
                "sha256_decoded": nscr_record.get("sha256_decoded"),
            },
        },
        "rendered_source_size": [width, height],
        "crop": list(crop) if crop is not None else None,
        "output_size": [out_width, out_height],
        "unique_rgb_colors": unique_colors(out_rgb),
        "output_png": png_path.name,
        "conversion_rule": (
            "Exact NCGR+NCLR+NSCR reconstruction; optional crop only; "
            "no redrawing, resampling, or synthetic replacement pixels."
        ),
    }
    metadata_path.write_text(
        json.dumps(metadata, indent=2) + "\n",
        encoding="utf-8",
    )

    print(
        f"Bound HGSS storage role {args.role!r}: "
        f"NCGR {args.ncgr}, NCLR {args.nclr}, NSCR {args.nscr}"
    )
    print(f"PNG: {png_path} ({out_width}x{out_height}, {metadata['unique_rgb_colors']} colors)")
    print(f"Metadata: {metadata_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
