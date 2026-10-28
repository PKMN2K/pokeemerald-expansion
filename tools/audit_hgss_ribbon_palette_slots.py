#!/usr/bin/env python3
"""Audit HGSS ribbon background palette conflicts with live Emerald UI.

Read-only gate: this deliberately does not remap retail colors or modify assets.
Run from the repository root: python3 tools/audit_hgss_ribbon_palette_slots.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/pokenav_ribbons_summary.c"
# The currently ROM-verified exporter trials all reference these NCLR banks.
HGSS_BG_BANKS = {0, 2, 4}
# 4bpp backgrounds share sixteen physical GBA BG palette banks (0 through 15).
TOTAL_BANKS = 16

def audit(source):
    # Inspect actual palette destinations, not palette tags for OBJ sprites.
    loads = re.findall(
        r"CopyPaletteIntoBufferUnfaded\s*\(\s*([^,]+?)\s*,\s*"
        r"BG_PLTT_ID\s*\(\s*(\d+)\s*\)",
        source,
    )
    owners = {}
    for asset, bank in loads:
        owners.setdefault(int(bank), []).append(asset.strip())
    conflicts = {bank: owners[bank] for bank in sorted(HGSS_BG_BANKS & owners.keys())}
    return owners, conflicts

def audit_pokenav_modules():
    # Include symbolic bank expressions rather than silently assuming they are free.
    pattern = re.compile(
        r'(?:CopyPaletteIntoBufferUnfaded|LoadPalette|LoadCompressedPalette)'
        r'\s*\([^;]*?BG_PLTT_ID\s*\(\s*([^()]+?)\s*\)', re.S)
    results = {}
    for module in sorted((ROOT / 'src').glob('pokenav*.c')):
        source = module.read_text()
        matches = []
        for match in pattern.finditer(source):
            matches.append((source.count('\n', 0, match.start()) + 1, match.group(1).strip()))
        if matches:
            results[module.name] = matches
    return results

def main():
    owners, conflicts = audit(SOURCE.read_text())
    print("Retail HGSS BG NCLR banks:", sorted(HGSS_BG_BANKS))
    print("Live ribbon UI BG palette loads:")
    for bank in sorted(owners):
        print(f"  GBA BG bank {bank}: {', '.join(owners[bank])}")
    print("Overlapping banks:", sorted(conflicts))
    for bank, assets in conflicts.items():
        print(f"  bank {bank} would overwrite: {', '.join(assets)}")
    free = sorted(set(range(TOTAL_BANKS)) - owners.keys())
    print("Not explicitly loaded by this source:", free)
    print("Direct BG palette loads across PokéNav modules:")
    for module, loads in audit_pokenav_modules().items():
        print(f"  {module}: " + ", ".join(f"L{line} bank {bank}" for line, bank in loads))
    print("Symbolic banks, helper writes and tilemap ownership require further audit.")
    # BG bank 0 hosts individual help-bar colour uploads in pokenav_main_menu.c.
    # Treat every literal destination across modules as reserved for this
    # conservative audit. Symbolic destinations are unresolved, NOT free.
    module_loads = audit_pokenav_modules()
    literal_banks = {int(bank) for loads in module_loads.values()
                     for _, bank in loads if bank.isdecimal()}
    literal_banks.add(0)  # Partial help-bar BG_PLTT_ID(0) colour writes.
    candidates = sorted(set(range(TOTAL_BANKS)) - set(owners) - literal_banks)
    symbolic = [(module, line, bank) for module, loads in module_loads.items()
                for line, bank in loads if not bank.isdecimal()]
    print("Conservatively excluded direct-load and help-bar banks:", sorted(literal_banks))
    print("Unclaimed by this limited literal-load inventory:", candidates)
    print("Unresolved symbolic destinations:", len(symbolic))
    print("BLOCKED: no --palette-map is approved by a source-only scan;")
    print("BG bank lifetimes, symbolic constants, and tilemap references")
    print("must be resolved before a collision-safe export or live wiring.")
    print("Note: unlisted banks are NOT certified free; other PokéNav/BG "
          "initialization and tilemap palette references must also be checked.")
    if conflicts:
        print("BLOCKED: do not wire exported NCLR directly to its original "
              "palette banks or remove legacy backgrounds.")
    else:
        print("No direct palette-load overlap found; visual and cross-module "
              "validation still required.")

if __name__ == "__main__":
    main()
