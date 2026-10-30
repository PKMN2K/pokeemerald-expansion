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

def audit_all_bg_palette_references():
    """Inventory every BG_PLTT_ID reference, including partial/helper writes.

    A reference is not proof of ownership or a safe free palette bank.
    """
    results = {}
    pattern = re.compile(r"BG_PLTT_ID\s*\(\s*([^()]+?)\s*\)")
    for module in sorted((ROOT / "src").glob("pokenav*.c")):
        source = module.read_text()
        references = [
            (source.count("\n", 0, match.start()) + 1, match.group(1).strip())
            for match in pattern.finditer(source)
        ]
        if references:
            results[module.name] = references
    return results

def numeric_bank(expression):
    """Parse direct decimal/hex bank literals; leave expressions unresolved."""
    value = expression.strip()
    while value.startswith("(") and value.endswith(")"):
        value = value[1:-1].strip()
    if not re.fullmatch(r"(?:0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]*", value):
        return None
    digits = re.sub(r"[uUlL]+$", "", value)
    return int(digits, 16 if digits.lower().startswith("0x") else 10)

def numeric_palette_macros(module):
    """Resolve only numeric macros local to a PokéNav source module."""
    definitions = {}
    for name, value in re.findall(
        r"^\s*#define\s+(\w+)\s+([^\r\n/]+)",
        (ROOT / "src" / module).read_text(), re.M
    ):
        numeric = numeric_bank(value)
        if numeric is not None:
            definitions[name] = numeric
    return definitions

def audit_tilemap_palette_calls():
    """Find source-level tilemap palette setters; do not infer bank lifetimes."""
    names = (
        "FillBgTilemapBufferRect_Palette0",
        "FillBgTilemapBufferRect",
        "CopyToBgTilemapBuffer",
        "CopyToBgTilemapBufferRect",
        "CopyToBgTilemapBufferRect_ChangePalette",
        "SetBgTilemapBuffer",
        "ChangeBgX",
    )
    pattern = re.compile(r"\b(" + "|".join(names) + r")\s*\(")
    results = {}
    for module in sorted((ROOT / "src").glob("pokenav*.c")):
        source = module.read_text()
        # Show the complete source statement where practical. A function name
        # alone conceals the palette argument and the source tilemap.
        hits = []
        for match in pattern.finditer(source):
            line = source.count("\n", 0, match.start()) + 1
            end = source.find(";", match.end())
            if end < 0 or end - match.start() > 512:
                statement = "<statement requires manual inspection>"
            else:
                statement = " ".join(source[match.start():end + 1].split())
            hits.append((line, match.group(1), statement))
        if hits:
            results[module.name] = hits
    return results

def audit_ribbon_tilemap_palette_bits(source):
    """Inspect the known ribbon tilemap encoder without assuming bank safety."""
    palette_tags = [
        int(value) for _, value in re.findall(
            r"^#define\s+(PALTAG_RIBBON_ICONS_[1-5])\s+(\d+)\s*$",
            source, re.M
        )
    ]
    base = re.search(r"#define\s+TO_PAL_OFFSET\(palNum\)\s+\(\(palNum\)\s*-\s*PALTAG_RIBBON_ICONS_1\)", source)
    encoder = re.search(
        r"palNum\s*=\s*sRibbonGfxData\[ribbonId\]\.palNumOffset\s*\+\s*(\d+)\s*;", source
    )
    packing = re.search(r"dst\[i\]\s*=\s*\(tileNum\s*\+\s*i\)\s*\|\s*\(palNum\s*<<\s*12\)", source)
    if len(palette_tags) != 5 or not base or not encoder or not packing:
        print("UNRESOLVED: ribbon BG1 palette-bit encoding changed; inspect manually.")
        return set()
    banks = {tag - palette_tags[0] + int(encoder.group(1)) for tag in palette_tags}
    print("Ribbon BG1 tilemap palette banks from source expression:", sorted(banks))
    print("These are tilemap references, not proof of available palette memory.")
    return {bank for bank in banks if 0 <= bank < TOTAL_BANKS}

def audit_ribbon_summary_bg2_tilemap():
    """Read the 32x20 BG2 4bpp tilemap's palette nibble from source bytes."""
    tilemap = ROOT / "graphics/pokenav/ribbons/summary_bg.bin"
    data = tilemap.read_bytes()
    expected_size = 32 * 20 * 2
    if len(data) != expected_size:
        print(f"UNRESOLVED: BG2 tilemap size {len(data)} != {expected_size}")
        return set()
    entries = [int.from_bytes(data[i:i + 2], "little")
               for i in range(0, len(data), 2)]
    banks = {entry >> 12 for entry in entries}
    print("Ribbon BG2 summary tilemap palette banks:", sorted(banks))
    if banks != {1}:
        print("UNRESOLVED: BG2 summary tilemap no longer exclusively uses")
        print("bank 1; revisit BG1/BG2 palette ownership before wiring.")
    print("BG2 tilemap bytes:", tilemap.relative_to(ROOT), "entries:", len(entries))
    print("BG2 palette bits are evidence of references, not safe allocation.")
    return banks

def audit_ribbon_window_palettes(source):
    """Report direct window palette users on the live ribbon summary screen."""
    pattern = re.compile(
        r"static\\s+const\\s+struct\\s+WindowTemplate\\s+(\\w+)"
        r"(?:\\[\\])?\\s*=\\s*\\{(.*?)\\};", re.S
    )
    windows = []
    for match in pattern.finditer(source):
        palette = re.search(r"\\.paletteNum\\s*=\\s*(\\d+)", match.group(2))
        bg = re.search(r"\\.bg\\s*=\\s*(\\d+)", match.group(2))
        if palette:
            windows.append((match.group(1), int(bg.group(1)) if bg else None,
                            int(palette.group(1))))
    print("Ribbon summary window palette assignments:")
    for name, bg, bank in windows:
        print(f"  {name}: BG{bg if bg is not None else '?'} bank {bank}")
    print("Window bank references also reserve shared physical BG palettes.")
    return {bank for _, _, bank in windows if 0 <= bank < TOTAL_BANKS}

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
    print("All BG_PLTT_ID references across PokéNav (including helper writes):")
    for module, references in audit_all_bg_palette_references().items():
        print(f"  {module}: " + ", ".join(
            f"L{line} bank/expression {bank}" for line, bank in references
        ))
    print("Tilemap-copy/fill palette ownership call sites (inspection required):")
    for module, calls in audit_tilemap_palette_calls().items():
        for line, function, statement in calls:
            print(f"  {module}:L{line} {function}: {statement}")
    print("SetBgTilemapBuffer identifies BG-to-tilemap-buffer associations;")
    print("CopyToBgTilemapBuffer may copy pre-encoded BG tilemap palette bits;")
    print("inspect its source tilemap data before approving any palette bank.")
    print("CopyToBgTilemapBufferRect may preserve palette bits from source data.")
    print("Call sites do not establish the palette values in map data.")
    print("References are diagnostic only; no banks are certified free.")
    print("Symbolic banks, helper writes and tilemap ownership require further audit.")
    # BG bank 0 hosts individual help-bar colour uploads in pokenav_main_menu.c.
    # Treat every literal destination across modules as reserved for this
    # conservative audit. Symbolic destinations are unresolved, NOT free.
    module_loads = audit_pokenav_modules()
    literal_banks = {value for loads in module_loads.values()
                     for _, bank in loads
                     if (value := numeric_bank(bank)) is not None and 0 <= value < TOTAL_BANKS}
    literal_banks.add(0)  # Partial help-bar BG_PLTT_ID(0) colour writes.
    bg1_banks = audit_ribbon_tilemap_palette_bits(SOURCE.read_text())
    bg2_banks = audit_ribbon_summary_bg2_tilemap()
    print("Ribbon BG1/BG2 shared tilemap palette banks:", sorted(bg1_banks & bg2_banks))
    if bg1_banks & bg2_banks:
        print("CONFLICT RISK: both BG layers reference the same physical BG")
        print("palette bank; verify their colors match before any replacement.")
    else:
        print("No shared BG1/BG2 tilemap bank references found; palette")
        print("loads, windows and other layers still require collision checks.")
    literal_banks.update(bg1_banks | bg2_banks)
    literal_banks.update(audit_ribbon_window_palettes(SOURCE.read_text()))
    # All explicit BG_PLTT_ID references may participate in palette use,
    # including writes outside the direct-load functions above. Reserve them.
    # Symbolic expressions remain unresolved and cannot certify any bank free.
    all_references = audit_all_bg_palette_references()
    resolved_references = []
    macro_definitions = {module: numeric_palette_macros(module) for module in all_references}
    for module, references in all_references.items():
        definitions = macro_definitions[module]
        for line, expression in references:
            value = numeric_bank(expression)
            if value is None:
                value = definitions.get(expression)
            if value is not None and 0 <= value < TOTAL_BANKS:
                literal_banks.add(value)
                if numeric_bank(expression) is None:
                    resolved_references.append((module, line, expression, value))
    symbolic = []
    resolved = []
    for module, loads in module_loads.items():
        definitions = numeric_palette_macros(module)
        for line, bank in loads:
            if numeric_bank(bank) is not None:
                continue
            if bank in definitions and 0 <= int(definitions[bank]) < TOTAL_BANKS:
                target = int(definitions[bank])
                literal_banks.add(target)
                resolved.append((module, line, bank, target))
            else:
                symbolic.append((module, line, bank))
    candidates = sorted(set(range(TOTAL_BANKS)) - set(owners) - literal_banks)
    print("Resolved symbolic destinations:")
    for module, line, name, bank in resolved:
        print(f"  {module}:L{line} {name} = bank {bank}")
    print("Resolved symbolic BG palette references:")
    for module, line, name, bank in resolved_references:
        print(f"  {module}:L{line} {name} = bank {bank}")
    print("Conservatively excluded direct-load and help-bar banks:", sorted(literal_banks))
    print("Unclaimed by this limited literal-load inventory:", candidates)
    unresolved_references = [
        (module, line, expression)
        for module, references in all_references.items()
        for line, expression in references
        if numeric_bank(expression) is None and expression not in macro_definitions[module]
    ]
    print("Nonliteral BG_PLTT_ID references requiring review:", len(unresolved_references))
    for module, line, expression in unresolved_references:
        print(f"  {module}:L{line} {expression}")
    print("Unresolved symbolic destinations:", len(symbolic))
    for module, line, name in symbolic:
        print(f"  {module}:L{line} {name}")
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
