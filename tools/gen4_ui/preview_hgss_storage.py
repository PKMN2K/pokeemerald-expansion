#!/usr/bin/env python3
"""
Build a visual identification gallery for authentic HGSS PC Storage assets.

The tool accepts the same user-provided HeartGold/SoulSilver .nds ROM or
extracted /a/0/1/9 NARC as map_hgss_storage_narc.py. It:

1. extracts and decodes all archive members;
2. parses every NCGR, NCLR and NSCR member;
3. forms only structurally compatible NCGR+NCLR+NSCR combinations;
4. writes the detailed archive/member manifest used by the standalone mapper;
5. renders the best candidates for each NSCR;
6. writes a JSON gallery index and an HTML contact sheet.

No semantic names are assigned automatically. The gallery exists so a human can
verify which authentic HGSS member is the box background, header, arrows, etc.
"""

from __future__ import annotations

import argparse
import html
import json
from pathlib import Path

import map_hgss_storage_narc as storage_map
import render_hgss_nitro as nitro


def decoded_members(source: storage_map.InputArchive):
    members = storage_map.parse_narc(source.data)
    out = []
    records = []
    for member in members:
        record, decoded = storage_map.member_record(member)
        records.append(record)
        out.append(
            {
                "index": member.index,
                "decoded": decoded,
                "compression": record["compression"],
                "type": record["type"],
            }
        )
    return out, records


def build_member_manifest(
    source: storage_map.InputArchive,
    records,
) -> dict[str, object]:
    manifest: dict[str, object] = {
        "source_archive": f"/{storage_map.HGSS_STORAGE_PATH}",
        "source_kind": source.source_kind,
        "source_file": source.source_path,
        "archive_sha256": storage_map.hashlib.sha256(source.data).hexdigest(),
        "member_count": len(records),
        "expected_hgss_member_count": 87,
        "count_matches_expected": len(records) == 87,
        "members": records,
    }

    if source.source_kind == "nds_rom":
        manifest["rom_title"] = source.rom_title
        manifest["rom_game_code"] = source.rom_game_code
        manifest["rom_file_id"] = source.rom_file_id
        manifest["rom_archive_offset_start"] = source.rom_offset_start
        manifest["rom_archive_offset_end"] = source.rom_offset_end

    return manifest


def write_member_files(members, member_dir: Path) -> dict[int, Path]:
    member_dir.mkdir(parents=True, exist_ok=True)
    ext_map = {
        "NCGR": ".ncgr",
        "NCLR": ".nclr",
        "NSCR": ".nscr",
        "NCER": ".ncer",
        "NANR": ".nanr",
        "NFTR": ".nftr",
        "NARC": ".narc",
        "BIN": ".bin",
        "EMPTY": ".bin",
    }
    paths: dict[int, Path] = {}
    for member in members:
        suffix = ext_map.get(member["type"], ".bin")
        path = member_dir / f'{member["index"]:03d}_{member["type"].lower()}{suffix}'
        path.write_bytes(member["decoded"])
        paths[member["index"]] = path
    return paths


def parse_formats(members, paths):
    ncgrs = {}
    nclrs = {}
    nscrs = {}
    errors = []

    for member in members:
        idx = member["index"]
        kind = member["type"]
        path = paths[idx]
        try:
            if kind == "NCGR":
                ncgrs[idx] = nitro.parse_ncgr(path)
            elif kind == "NCLR":
                nclrs[idx] = nitro.parse_nclr(path)
            elif kind == "NSCR":
                nscrs[idx] = nitro.parse_nscr(path)
        except ValueError as exc:
            errors.append({"index": idx, "type": kind, "error": str(exc)})

    return ncgrs, nclrs, nscrs, errors


def palette_capacity_ok(nclr: nitro.Nclr, nscr: nitro.Nscr) -> bool:
    if nscr.bit_depth == 8:
        return len(nclr.colors) >= 256

    max_bank = 0
    for entry in nscr.entries:
        max_bank = max(max_bank, (entry >> 12) & 0x0F)
    return len(nclr.colors) >= (max_bank + 1) * 16


def tileset_capacity_ok(ncgr: nitro.Ncgr, nscr: nitro.Nscr) -> bool:
    if ncgr.scanned or ncgr.bit_depth != nscr.bit_depth:
        return False
    if not nscr.entries:
        return False
    max_tile = max(entry & 0x03FF for entry in nscr.entries)
    return max_tile < len(ncgr.tiles)


def candidate_score(nscr_idx: int, ncgr_idx: int, nclr_idx: int) -> tuple[int, int, int, int]:
    # Archive adjacency is not treated as semantic proof; it is only a stable way
    # to put likely companion files near the top of a finite candidate gallery.
    span = max(nscr_idx, ncgr_idx, nclr_idx) - min(nscr_idx, ncgr_idx, nclr_idx)
    distance = abs(nscr_idx - ncgr_idx) + abs(nscr_idx - nclr_idx)
    return (span, distance, ncgr_idx, nclr_idx)


def render_candidates(
    ncgrs,
    nclrs,
    nscrs,
    preview_dir: Path,
    max_pairs_per_screen: int,
):
    preview_dir.mkdir(parents=True, exist_ok=True)
    records = []

    for nscr_idx, nscr in sorted(nscrs.items()):
        compatible_ncgrs = [
            (idx, obj)
            for idx, obj in ncgrs.items()
            if tileset_capacity_ok(obj, nscr)
        ]
        compatible_nclrs = [
            (idx, obj)
            for idx, obj in nclrs.items()
            if palette_capacity_ok(obj, nscr)
        ]

        pairs = []
        for ncgr_idx, ncgr in compatible_ncgrs:
            for nclr_idx, nclr in compatible_nclrs:
                pairs.append(
                    (
                        candidate_score(nscr_idx, ncgr_idx, nclr_idx),
                        ncgr_idx,
                        ncgr,
                        nclr_idx,
                        nclr,
                    )
                )
        pairs.sort(key=lambda item: item[0])

        screen_record = {
            "nscr_index": nscr_idx,
            "bit_depth": nscr.bit_depth,
            "width_tiles": nscr.width_tiles,
            "height_tiles": nscr.height_tiles,
            "compatible_ncgr_count": len(compatible_ncgrs),
            "compatible_nclr_count": len(compatible_nclrs),
            "candidate_count": len(pairs),
            "previews": [],
        }

        for _, ncgr_idx, ncgr, nclr_idx, nclr in pairs[:max_pairs_per_screen]:
            filename = (
                f"nscr_{nscr_idx:03d}__ncgr_{ncgr_idx:03d}"
                f"__nclr_{nclr_idx:03d}.png"
            )
            output = preview_dir / filename
            try:
                width, height, rgb = nitro.render_screen(ncgr, nclr, nscr)
                nitro.write_rgb_png(output, width, height, rgb)
            except ValueError as exc:
                screen_record["previews"].append(
                    {
                        "ncgr_index": ncgr_idx,
                        "nclr_index": nclr_idx,
                        "error": str(exc),
                    }
                )
                continue

            screen_record["previews"].append(
                {
                    "ncgr_index": ncgr_idx,
                    "nclr_index": nclr_idx,
                    "file": filename,
                    "width": width,
                    "height": height,
                }
            )

        records.append(screen_record)

    return records


def build_html(index, output_dir: Path) -> None:
    cards = []
    for screen in index["screens"]:
        nscr_idx = screen["nscr_index"]
        previews = [p for p in screen["previews"] if "file" in p]

        if not previews:
            cards.append(
                f"""
                <section class="screen">
                  <h2>NSCR {nscr_idx:03d}</h2>
                  <p>No structurally valid preview combination was found.</p>
                </section>
                """
            )
            continue

        preview_html = []
        for preview in previews:
            label = (
                f'NSCR {nscr_idx:03d} + '
                f'NCGR {preview["ncgr_index"]:03d} + '
                f'NCLR {preview["nclr_index"]:03d}'
            )
            preview_html.append(
                f"""
                <figure>
                  <img src="previews/{html.escape(preview["file"])}"
                       alt="{html.escape(label)}"
                       loading="lazy">
                  <figcaption>{html.escape(label)}<br>
                    {preview["width"]}x{preview["height"]} px
                  </figcaption>
                </figure>
                """
            )

        cards.append(
            f"""
            <section class="screen">
              <h2>NSCR {nscr_idx:03d}</h2>
              <p>
                {screen["bit_depth"]}bpp ·
                {screen["width_tiles"]}x{screen["height_tiles"]} tiles ·
                {screen["candidate_count"]} structurally valid combinations
              </p>
              <div class="grid">
                {''.join(preview_html)}
              </div>
            </section>
            """
        )

    document = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HGSS Storage /a/0/1/9 Visual Member Map</title>
<style>
body {{
  margin: 0;
  padding: 24px;
  font-family: system-ui, sans-serif;
  background: #181818;
  color: #f2f2f2;
}}
h1 {{ margin-top: 0; }}
.note {{
  max-width: 900px;
  padding: 12px 16px;
  background: #272727;
  border-radius: 8px;
}}
.screen {{
  margin: 28px 0;
  padding-top: 12px;
  border-top: 1px solid #555;
}}
.grid {{
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 16px;
}}
figure {{
  margin: 0;
  padding: 12px;
  background: #242424;
  border-radius: 8px;
}}
img {{
  display: block;
  max-width: 100%;
  height: auto;
  image-rendering: pixelated;
  background: #000;
  margin: 0 auto 8px;
}}
figcaption {{
  font-family: ui-monospace, monospace;
  font-size: 13px;
  line-height: 1.4;
}}
</style>
</head>
<body>
<h1>HGSS Storage /a/0/1/9 Visual Member Map</h1>
<div class="note">
<p><strong>Identification aid only.</strong> Candidate ordering uses archive
proximity after structural validation. It does not assign semantic roles.</p>
<p>Source archive SHA-256:
<code>{html.escape(index["archive_sha256"])}</code></p>
<p>Members: {index["member_count"]};
NCGR: {index["format_counts"]["NCGR"]};
NCLR: {index["format_counts"]["NCLR"]};
NSCR: {index["format_counts"]["NSCR"]}</p>
</div>
{''.join(cards)}
</body>
</html>
"""
    (output_dir / "index.html").write_text(document, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build a visual candidate gallery for authentic HGSS PC Storage assets"
    )
    parser.add_argument(
        "input",
        type=Path,
        help="HeartGold/SoulSilver .nds or extracted /a/0/1/9 NARC",
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Output directory for members, previews, JSON index, and HTML gallery",
    )
    parser.add_argument(
        "--max-pairs-per-screen",
        type=int,
        default=12,
        help="Maximum structurally valid previews per NSCR (default: 12)",
    )
    args = parser.parse_args()

    if args.max_pairs_per_screen <= 0:
        parser.error("--max-pairs-per-screen must be positive")

    source = storage_map.read_input_archive(args.input)
    members, member_records = decoded_members(source)

    if len(members) != 87:
        raise SystemExit(
            f"Expected 87 members in /{storage_map.HGSS_STORAGE_PATH}; found {len(members)}"
        )

    args.output.mkdir(parents=True, exist_ok=True)
    paths = write_member_files(members, args.output / "members")
    ncgrs, nclrs, nscrs, parse_errors = parse_formats(members, paths)

    screens = render_candidates(
        ncgrs,
        nclrs,
        nscrs,
        args.output / "previews",
        args.max_pairs_per_screen,
    )

    manifest = build_member_manifest(source, member_records)
    manifest_path = args.output / "manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2) + "\n",
        encoding="utf-8",
    )

    index = {
        "source_archive": manifest["source_archive"],
        "source_kind": manifest["source_kind"],
        "source_file": manifest["source_file"],
        "archive_sha256": manifest["archive_sha256"],
        "member_count": len(members),
        "format_counts": {
            "NCGR": len(ncgrs),
            "NCLR": len(nclrs),
            "NSCR": len(nscrs),
        },
        "parse_errors": parse_errors,
        "screens": screens,
    }

    (args.output / "index.json").write_text(
        json.dumps(index, indent=2) + "\n",
        encoding="utf-8",
    )
    build_html(index, args.output)

    preview_count = sum(
        1
        for screen in screens
        for preview in screen["previews"]
        if "file" in preview
    )
    print(f"Decoded {len(members)} members from /{storage_map.HGSS_STORAGE_PATH}")
    print(
        f"Found {len(ncgrs)} NCGR, {len(nclrs)} NCLR, "
        f"and {len(nscrs)} NSCR members"
    )
    print(f"Rendered {preview_count} structurally valid candidate previews")
    print(f"Manifest: {manifest_path}")
    print(f"Gallery: {args.output / 'index.html'}")
    print(f"Index: {args.output / 'index.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
